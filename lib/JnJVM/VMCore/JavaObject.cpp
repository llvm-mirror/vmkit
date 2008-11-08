//===----------- JavaObject.cpp - Java object definition ------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/JIT.h"
#include "mvm/Threads/Locks.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

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

LockObj* LockObj::allocate() {
#ifdef USE_GC_BOEHM
  LockObj* res = new LockObj();
#else
  LockObj* res = gc_new(LockObj)();
#endif
  return res;
}

bool JavaObject::owner() {
  uint32 id = mvm::Thread::get()->threadID;
  if (id == lock) return true;
  if ((lock & 0x7FFFFF00) == id) return true;
  if (lock & 0x80000000) {
    LockObj* obj = (LockObj*)(lock << 1);
    return obj->owner();
  }
  return false;
}

void JavaObject::overflowThinlock() {
  LockObj* obj = LockObj::allocate();
  obj->lock.lockAll(257);
  lock = ((uint32)obj >> 1) | 0x80000000;
}

void JavaObject::release() {
  uint32 id = mvm::Thread::get()->threadID;
  if (lock == id) {
    lock = 0;
  } else if (lock & 0x80000000) {
    LockObj* obj = (LockObj*)(lock << 1);
    obj->release();
  } else {
    lock--;
  }
}

void JavaObject::acquire() {
  uint32 id = mvm::Thread::get()->threadID;
  uint32 val = __sync_val_compare_and_swap((uint32*)&lock, 0, id);
  if (val != 0) {
    //fat!
    if (!(val & 0x80000000)) {
      if ((val & 0x7FFFFF00) == id) {
        if ((val & 0xFF) != 0xFF) {
          lock++;
        } else {
          overflowThinlock();
        }
      } else {
        LockObj* obj = LockObj::allocate();
        uint32 val = ((uint32)obj >> 1) | 0x80000000;
loop:
        uint32 count = 0;
        while (lock) {
          if (lock & 0x80000000) {
#ifdef USE_GC_BOEHM
            delete obj;
#endif
            goto end;
          }
          else mvm::Thread::yield(&count);
        }
        
        uint32 test = __sync_val_compare_and_swap((uint32*)&lock, 0, val);
        if (test) goto loop;
        obj->acquire();
      }
    } else {
end:
      LockObj* obj = (LockObj*)(lock << 1);
      obj->acquire();
    }
  }
}

LockObj* JavaObject::changeToFatlock() {
  if (!(lock & 0x80000000)) {
    LockObj* obj = LockObj::allocate();
    uint32 val = (((uint32) obj) >> 1) | 0x80000000;
    uint32 count = lock & 0xFF;
    obj->lock.lockAll(count + 1);
    lock = val;
    return obj;
  } else {
    return (LockObj*)(lock << 1);
  }
}

void JavaObject::print(mvm::PrintBuffer* buf) const {
  buf->write("JavaObject<");
  CommonClass::printClassName(classOf->getName(), buf);
  buf->write(">");
}

void JavaObject::waitIntern(struct timeval* info, bool timed) {

  if (owner()) {
    LockObj * l = changeToFatlock();
    JavaThread* thread = JavaThread::get();
    mvm::Lock& mutexThread = thread->lock;
    mvm::Cond& varcondThread = thread->varcond;

    mutexThread.lock();
    if (thread->interruptFlag != 0) {
      mutexThread.unlock();
      thread->interruptFlag = 0;
      thread->isolate->interruptedException(this);
    } else {
      unsigned int recur = l->lock.recursionCount();
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
        thread->isolate->interruptedException(this);
      }
    }
  } else {
    JavaThread::get()->isolate->illegalMonitorStateException(this);
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
    LockObj * l = changeToFatlock();
    l->getCond()->notify();
  } else {
    JavaThread::get()->isolate->illegalMonitorStateException(this);
  }
}

void JavaObject::notifyAll() {
  if (owner()) {
    LockObj * l = changeToFatlock();
    l->getCond()->notifyAll();
  } else {
    JavaThread::get()->isolate->illegalMonitorStateException(this);
  } 
}
