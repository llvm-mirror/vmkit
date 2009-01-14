//===----------- JavaObject.cpp - Java object definition ------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/Threads/Locks.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace jnjvm;

void JavaCond::notify() {
  for (std::vector<JavaThread*>::iterator i = threads.begin(), 
            e = threads.end(); i!= e;) {
    JavaThread* cur = *i;
    cur->lock.lock();
    if (cur->interruptFlag != 0) {
      cur->lock.unlock();
      ++i;
      continue;
    } else if (cur->javaThread != 0) {
      cur->varcond.signal();
      cur->lock.unlock();
      threads.erase(i);
      break;
    } else { // dead thread
      ++i;
      threads.erase(i - 1);
    }
  }
}

void JavaCond::notifyAll() {
  for (std::vector<JavaThread*>::iterator i = threads.begin(),
            e = threads.end(); i!= e; ++i) {
    JavaThread* cur = *i;
    cur->lock.lock();
    cur->varcond.signal();
    cur->lock.unlock();
  }
  threads.clear();
}

void JavaCond::wait(JavaThread* th) {
  threads.push_back(th);
}

void JavaCond::remove(JavaThread* th) {
  for (std::vector<JavaThread*>::iterator i = threads.begin(),
            e = threads.end(); i!= e; ++i) {
    if (*i == th) {
      threads.erase(i);
      break;
    }
  }
}

LockObj* LockObj::allocate(JavaObject* owner) {
#ifdef USE_GC_BOEHM
  LockObj* res = new LockObj();
#else
  LockObj* res = gc_new(LockObj)();
#endif
  return res;
}

extern "C" void printJavaObject(const JavaObject* obj, mvm::PrintBuffer* buf) {
  buf->write("JavaObject<");
  CommonClass::printClassName(obj->classOf->getName(), buf);
  fprintf(stderr, "%p\n", ((void**)obj->getVirtualTable())[9]);
  buf->write(">");
}

void JavaObject::print(mvm::PrintBuffer* buf) const {
  printJavaObject(this, buf);
}


void JavaObject::waitIntern(struct timeval* info, bool timed) {

  if (owner()) {
    LockObj * l = lock.changeToFatlock(this);
    JavaThread* thread = JavaThread::get();
    mvm::Lock& mutexThread = thread->lock;
    mvm::Cond& varcondThread = thread->varcond;

    mutexThread.lock();
    if (thread->interruptFlag != 0) {
      mutexThread.unlock();
      thread->interruptFlag = 0;
      thread->getJVM()->interruptedException(this);
    } else {
      uint32_t recur = l->lock.recursionCount();
      bool timeout = false;
      l->lock.unlockAll();
      JavaCond* cond = l->getCond();
      cond->wait(thread);
      thread->state = JavaThread::StateWaiting;

      if (timed) {
        timeout = varcondThread.timedWait(&mutexThread, info);
      } else {
        varcondThread.wait(&mutexThread);
      }

      bool interrupted = (thread->interruptFlag != 0);
      mutexThread.unlock();
      l->lock.lockAll(recur);

      if (interrupted || timeout) {
        cond->remove(thread);
      }

      thread->state = JavaThread::StateRunning;

      if (interrupted) {
        thread->interruptFlag = 0;
        thread->getJVM()->interruptedException(this);
      }
    }
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(this);
  }
}

void JavaObject::wait() {
  waitIntern(0, false);
}

void JavaObject::timedWait(struct timeval& info) {
  waitIntern(&info, true);
}

void JavaObject::notify() {
  if (owner()) {
    LockObj * l = lock.getFatLock();
    if (l) l->getCond()->notify();
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(this);
  }
}

void JavaObject::notifyAll() {
  if (owner()) {
    LockObj * l = lock.getFatLock();
    if (l) l->getCond()->notifyAll();
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(this);
  } 
}

void JavaObject::decapsulePrimitive(Jnjvm *vm, uintptr_t &buf,
                                    const Typedef* signature) {

  JavaObject* obj = this;
  if (!signature->isPrimitive()) {
    if (obj && !(obj->classOf->isOfTypeName(vm, signature->getName()))) {
      vm->illegalArgumentException("wrong type argument");
    }
    ((JavaObject**)buf)[0] = obj;
    buf += 8;
    return;
  } else if (obj == 0) {
    vm->illegalArgumentException("");
  } else {
    UserCommonClass* cl = obj->classOf;
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
