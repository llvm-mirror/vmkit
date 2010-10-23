//===----------- JavaObject.cpp - Java object definition ------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Locks.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

#include <jni.h>

using namespace j3;

static const int hashCodeIncrement = mvm::GCBitMask + 1;
uint16_t JavaObject::hashCodeGenerator = hashCodeIncrement;

/// hashCode - Return the hash code of this object.
uint32_t JavaObject::hashCode(JavaObject* self) {
  llvm_gcroot(self, 0);
  if (!mvm::MovesObject) return (uint32_t)(long)self;

  uintptr_t header = self->header;
  uintptr_t GCBits = header & mvm::GCBitMask;
  uintptr_t val = header & mvm::HashMask;
  if (val != 0) {
    return val ^ (uintptr_t)getClass(self);
  }
  val = hashCodeGenerator;
  hashCodeGenerator += hashCodeIncrement;
  val = val % mvm::HashMask;
  if (val == 0) {
    // It is possible that in the same time, a thread is in this method and
    // gets the same hash code value than this thread. This is fine.
    val = hashCodeIncrement;
    hashCodeGenerator += hashCodeIncrement;
  }
  assert(val > mvm::GCBitMask);
  assert(val <= mvm::HashMask);

  do {
    header = self->header;
    if ((header & mvm::HashMask) != 0) break;
    uintptr_t newHeader = header | val;
    assert((newHeader & ~mvm::HashMask) == header);
    __sync_val_compare_and_swap(&(self->header), header, newHeader);
  } while (true);

  assert((self->header & mvm::HashMask) != 0);
  assert(GCBits == (self->header & mvm::GCBitMask));
  return (self->header & mvm::HashMask) ^ (uintptr_t)getClass(self);
}


void JavaObject::waitIntern(
    JavaObject* self, struct timeval* info, bool timed) {
  llvm_gcroot(self, 0);
  JavaLock* l = 0;

  if (owner(self)) {
    l = (JavaLock*)mvm::ThinLock::changeToFatlock(self);
    JavaThread* thread = JavaThread::get();
    thread->waitsOn = l;
    mvm::Cond& varcondThread = thread->varcond;

    if (thread->interruptFlag != 0) {
      thread->interruptFlag = 0;
      thread->waitsOn = 0;
      thread->getJVM()->interruptedException(self);
    } else { 
      thread->state = JavaThread::StateWaiting;
      if (l->firstThread) {
        assert(l->firstThread->prevWaiting && l->firstThread->nextWaiting &&
               "Inconsistent list");
        if (l->firstThread->nextWaiting == l->firstThread) {
          l->firstThread->nextWaiting = thread;
        } else {
          l->firstThread->prevWaiting->nextWaiting = thread;
        } 
        thread->prevWaiting = l->firstThread->prevWaiting;
        thread->nextWaiting = l->firstThread;
        l->firstThread->prevWaiting = thread;
      } else {
        l->firstThread = thread;
        thread->nextWaiting = thread;
        thread->prevWaiting = thread;
      }
      assert(thread->prevWaiting && thread->nextWaiting && "Inconsistent list");
      assert(l->firstThread->prevWaiting && l->firstThread->nextWaiting &&
             "Inconsistent list");
      
      bool timeout = false;

      l->waitingThreads++;

      while (!thread->interruptFlag && thread->nextWaiting) {
        if (timed) {
          timeout = varcondThread.timedWait(&l->internalLock, info);
          if (timeout) break;
        } else {
          varcondThread.wait(&l->internalLock);
        }
      }
      
      l->waitingThreads--;
     
      assert((!l->firstThread || (l->firstThread->prevWaiting && 
             l->firstThread->nextWaiting)) && "Inconsistent list");
 
      bool interrupted = (thread->interruptFlag != 0);

      if (interrupted || timeout) {
        
        if (thread->nextWaiting) {
          assert(thread->prevWaiting && "Inconsistent list");
          if (l->firstThread != thread) {
            thread->nextWaiting->prevWaiting = thread->prevWaiting;
            thread->prevWaiting->nextWaiting = thread->nextWaiting;
            assert(l->firstThread->prevWaiting && 
                   l->firstThread->nextWaiting && "Inconsistent list");
          } else if (thread->nextWaiting == thread) {
            l->firstThread = 0;
          } else {
            l->firstThread = thread->nextWaiting;
            l->firstThread->prevWaiting = thread->prevWaiting;
            thread->prevWaiting->nextWaiting = l->firstThread;
            assert(l->firstThread->prevWaiting && 
                   l->firstThread->nextWaiting && "Inconsistent list");
          }
          thread->nextWaiting = 0;
          thread->prevWaiting = 0;
        } else {
          assert(!thread->prevWaiting && "Inconstitent state");
          // Notify lost, notify someone else.
          notify(self);
        }
      } else {
        assert(!thread->prevWaiting && !thread->nextWaiting &&
               "Inconsistent state");
      }
      
      thread->state = JavaThread::StateRunning;
      thread->waitsOn = 0;

      if (interrupted) {
        thread->interruptFlag = 0;
        thread->getJVM()->interruptedException(self);
      }
    }
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(self);
  }
  
  assert(owner(self) && "Not owner after wait");
}

void JavaObject::wait(JavaObject* self) {
  llvm_gcroot(self, 0);
  waitIntern(self, NULL, false);
}

void JavaObject::timedWait(JavaObject* self, struct timeval& info) {
  llvm_gcroot(self, 0);
  waitIntern(self, &info, true);
}

void JavaObject::notify(JavaObject* self) {
  llvm_gcroot(self, 0);
  JavaLock* l = 0;

  if (owner(self)) {
    l = (JavaLock*)mvm::ThinLock::getFatLock(self);
    if (l) {
      JavaThread* cur = l->firstThread;
      if (cur) {
        do {
          if (cur->interruptFlag != 0) {
            cur = cur->nextWaiting;
          } else {
            assert(cur->javaThread && "No java thread");
            assert(cur->prevWaiting && cur->nextWaiting &&
                   "Inconsistent list");
            if (cur != l->firstThread) {
              cur->prevWaiting->nextWaiting = cur->nextWaiting;
              cur->nextWaiting->prevWaiting = cur->prevWaiting;
              assert(l->firstThread->prevWaiting &&
                     l->firstThread->nextWaiting && "Inconsistent list");
            } else if (cur->nextWaiting == cur) {
              l->firstThread = 0;
            } else {
              l->firstThread = cur->nextWaiting;
              l->firstThread->prevWaiting = cur->prevWaiting;
              cur->prevWaiting->nextWaiting = l->firstThread;
              assert(l->firstThread->prevWaiting && 
                     l->firstThread->nextWaiting && "Inconsistent list");
            }
            cur->prevWaiting = 0;
            cur->nextWaiting = 0;
            cur->varcond.signal();
            break;
          }
        } while (cur != l->firstThread);
      }
    }
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(self);
  }
  assert(owner(self) && "Not owner after notify");
}

void JavaObject::notifyAll(JavaObject* self) {
  llvm_gcroot(self, 0);
  JavaLock* l = 0;
  
  if (owner(self)) {
    l = (JavaLock*)mvm::ThinLock::getFatLock(self);
    if (l) {
      JavaThread* cur = l->firstThread;
      if (cur) {
        do {
          JavaThread* temp = cur->nextWaiting;
          cur->prevWaiting = 0;
          cur->nextWaiting = 0;
          cur->varcond.signal();
          cur = temp;
        } while (cur != l->firstThread);
        l->firstThread = 0;
      }
    }
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(self);
  }

  assert(owner(self) && "Not owner after notifyAll");
}

void JavaObject::decapsulePrimitive(JavaObject* obj, Jnjvm *vm, jvalue* buf,
                                    const Typedef* signature) {

  llvm_gcroot(obj, 0);

  if (!signature->isPrimitive()) {
    if (obj && !(getClass(obj)->isOfTypeName(signature->getName()))) {
      vm->illegalArgumentException("wrong type argument");
    }
    return;
  } else if (obj == NULL) {
    vm->illegalArgumentException("");
  } else {
    UserCommonClass* cl = getClass(obj);
    UserClassPrimitive* value = cl->toPrimitive(vm);
    const PrimitiveTypedef* prim = (const PrimitiveTypedef*)signature;

    if (value == 0) {
      vm->illegalArgumentException("");
    }
    
    if (prim->isShort()) {
      if (value == vm->upcalls->OfShort) {
        (*buf).s = vm->upcalls->shortValue->getInstanceInt16Field(obj);
        return;
      } else if (value == vm->upcalls->OfByte) {
        (*buf).s = (sint16)vm->upcalls->byteValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isByte()) {
      if (value == vm->upcalls->OfByte) {
        (*buf).b = vm->upcalls->byteValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isBool()) {
      if (value == vm->upcalls->OfBool) {
        (*buf).z = vm->upcalls->boolValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isInt()) {
      if (value == vm->upcalls->OfInt) {
        (*buf).i = vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).i = (sint32)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).i = (uint32)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).i = (sint32)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isChar()) {
      if (value == vm->upcalls->OfChar) {
        (*buf).c = (uint16)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isFloat()) {
      if (value == vm->upcalls->OfFloat) {
        (*buf).f = (float)vm->upcalls->floatValue->getInstanceFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).f = (float)(sint32)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).f = (float)(uint32)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).f = (float)(sint32)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).f = (float)(sint32)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).f = (float)vm->upcalls->longValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isDouble()) {
      if (value == vm->upcalls->OfDouble) {
        (*buf).d = (double)vm->upcalls->doubleValue->getInstanceDoubleField(obj);
      } else if (value == vm->upcalls->OfFloat) {
        (*buf).d = (double)vm->upcalls->floatValue->getInstanceFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).d = (double)(sint64)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).d = (double)(uint64)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).d = (double)(sint16)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).d = (double)(sint32)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).d = (double)(sint64)vm->upcalls->longValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isLong()) {
      if (value == vm->upcalls->OfByte) {
        (*buf).j = (sint64)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).j = (sint64)(uint64)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).j = (sint64)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).j = (sint64)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).j = (sint64)vm->upcalls->intValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    }
  }
  // can not be here
  return;
}

bool JavaObject::instanceOf(JavaObject* self, UserCommonClass* cl) {
  llvm_gcroot(self, 0);
  if (self == NULL) return false;
  else return getClass(self)->isAssignableFrom(cl);
}
