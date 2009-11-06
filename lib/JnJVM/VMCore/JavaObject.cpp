//===----------- JavaObject.cpp - Java object definition ------------------===//
//
//                              JnJVM
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

using namespace jnjvm;

void JavaObject::waitIntern(struct timeval* info, bool timed) {
  JavaLock* l = 0;
  JavaObject* self = this;
  llvm_gcroot(self, 0);

  if (owner()) {
    l = self->lock.changeToFatlock(self);
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
          notify();
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
  
  assert(owner() && "Not owner after wait");
}

void JavaObject::wait() {
  JavaObject* self = this;
  llvm_gcroot(self, 0);
  self->waitIntern(0, false);
}

void JavaObject::timedWait(struct timeval& info) {
  JavaObject* self = this;
  llvm_gcroot(self, 0);
  self->waitIntern(&info, true);
}

void JavaObject::notify() {
  JavaLock* l = 0;
  JavaObject* self = this;
  llvm_gcroot(self, 0);

  if (owner()) {
    l = self->lock.getFatLock();
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
  assert(owner() && "Not owner after notify");
}

void JavaObject::notifyAll() {
  JavaLock* l = 0;
  JavaObject* self = this;
  llvm_gcroot(self, 0);
  
  if (owner()) {
    l = self->lock.getFatLock();
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

  assert(owner() && "Not owner after notifyAll");
}

void JavaObject::decapsulePrimitive(Jnjvm *vm, uintptr_t &buf,
                                    const Typedef* signature) {

  JavaObject* obj = this;
  llvm_gcroot(obj, 0);

  if (!signature->isPrimitive()) {
    if (obj && !(obj->getClass()->isOfTypeName(signature->getName()))) {
      vm->illegalArgumentException("wrong type argument");
    }
    ((JavaObject**)buf)[0] = obj;
    buf += 8;
    return;
  } else if (obj == 0) {
    vm->illegalArgumentException("");
  } else {
    UserCommonClass* cl = obj->getClass();
    UserClassPrimitive* value = cl->toPrimitive(vm);
    PrimitiveTypedef* prim = (PrimitiveTypedef*)signature;

    if (value == 0) {
      vm->illegalArgumentException("");
    }
    
    if (prim->isShort()) {
      if (value == vm->upcalls->OfShort) {
        ((uint16*)buf)[0] = vm->upcalls->shortValue->getInt16Field(obj);
        buf += 8;
        return;
      } else if (value == vm->upcalls->OfByte) {
        ((sint16*)buf)[0] = 
          (sint16)vm->upcalls->byteValue->getInt8Field(obj);
        buf += 8;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isByte()) {
      if (value == vm->upcalls->OfByte) {
        ((uint8*)buf)[0] = vm->upcalls->byteValue->getInt8Field(obj);
        buf += 8;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isBool()) {
      if (value == vm->upcalls->OfBool) {
        ((uint8*)buf)[0] = vm->upcalls->boolValue->getInt8Field(obj);
        buf += 8;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isInt()) {
      sint32 val = 0;
      if (value == vm->upcalls->OfInt) {
        val = vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (sint32)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (uint32)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (sint32)vm->upcalls->shortValue->getInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((sint32*)buf)[0] = val;
      buf += 8;
      return;
    } else if (prim->isChar()) {
      uint16 val = 0;
      if (value == vm->upcalls->OfChar) {
        val = (uint16)vm->upcalls->charValue->getInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((uint16*)buf)[0] = val;
      buf += 8;
      return;
    } else if (prim->isFloat()) {
      float val = 0;
      if (value == vm->upcalls->OfFloat) {
        val = (float)vm->upcalls->floatValue->getFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (float)(sint32)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (float)(uint32)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (float)(sint32)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (float)(sint32)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (float)vm->upcalls->longValue->getLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((float*)buf)[0] = val;
      buf += 8;
      return;
    } else if (prim->isDouble()) {
      double val = 0;
      if (value == vm->upcalls->OfDouble) {
        val = (double)vm->upcalls->doubleValue->getDoubleField(obj);
      } else if (value == vm->upcalls->OfFloat) {
        val = (double)vm->upcalls->floatValue->getFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (double)(sint64)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (double)(uint64)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (double)(sint16)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (double)(sint32)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (double)(sint64)vm->upcalls->longValue->getLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((double*)buf)[0] = val;
      buf += 8;
      return;
    } else if (prim->isLong()) {
      sint64 val = 0;
      if (value == vm->upcalls->OfByte) {
        val = (sint64)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (sint64)(uint64)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (sint64)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (sint64)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (sint64)vm->upcalls->intValue->getLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((sint64*)buf)[0] = val;
      buf += 8;
      return;
    }
  }
  // can not be here
  return;
}

bool JavaObject::instanceOf(UserCommonClass* cl) {
  JavaObject* self = this;
  llvm_gcroot(self, 0);

  if (!self) return false;
  else return self->getClass()->isAssignableFrom(cl);
}
