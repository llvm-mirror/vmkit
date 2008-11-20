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
  uint64 id = mvm::Thread::get()->getThreadID();
  if ((lock & ThinMask) == id) return true;
  if (lock & FatMask) {
    LockObj* obj = (LockObj*)(lock << 1);
    return obj->owner();
  }
  return false;
}

void JavaObject::overflowThinlock() {
  LockObj* obj = LockObj::allocate();
  obj->lock.lockAll(256);
  lock = ((uintptr_t)obj >> 1) | FatMask;
}

void JavaObject::release() {
  uint64 id = mvm::Thread::get()->getThreadID();
  if ((lock & ThinMask) == id) {
    --lock;
  } else {
    LockObj* obj = (LockObj*)(lock << 1);
    obj->release();
  } 
}

void JavaObject::acquire() {
  uint64_t id = mvm::Thread::get()->getThreadID();
  if ((lock & ReservedMask) == id) {
    lock |= 1;
  } else if ((lock & ThinMask) == id) {
    if ((lock & ThinCountMask) == ThinCountMask) {
      overflowThinlock();
    } else {
      ++lock;
    }
  } else {
    uintptr_t currentLock = lock & ThinMask;
    uintptr_t val = 
      (uintptr_t)__sync_val_compare_and_swap((uintptr_t)&lock, currentLock, 
                                             (id + 1));
    if (val != currentLock) {
      if (val & FatMask) {
end:
        //fat lock!
        LockObj* obj = (LockObj*)(lock << 1);
        obj->acquire();
      } else {
        LockObj* obj = LockObj::allocate();
        val = ((uintptr_t)obj >> 1) | FatMask;
        uint32 count = 0;
loop:
        while ((lock & ThinCountMask) != 0) {
          if (lock & FatMask) {
#ifdef USE_GC_BOEHM
            delete obj;
#endif
            goto end;
          }
          else {
            mvm::Thread::yield(&count);
          }
        }
        
        currentLock = lock & ThinMask;
        uintptr_t test = 
          (uintptr_t)__sync_val_compare_and_swap((uintptr_t)&lock, currentLock,
                                                 val);
        if (test != currentLock) goto loop;
        obj->acquire();
      }
    }
  }
}

LockObj* JavaObject::changeToFatlock() {
  if (!(lock & FatMask)) {
    LockObj* obj = LockObj::allocate();
    uintptr_t val = ((uintptr_t)obj >> 1) | FatMask;
    uint32 count = lock & ThinCountMask;
    obj->lock.lockAll(count);
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
    LockObj * l = changeToFatlock();
    l->getCond()->notify();
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(this);
  }
}

void JavaObject::notifyAll() {
  if (owner()) {
    LockObj * l = changeToFatlock();
    l->getCond()->notifyAll();
  } else {
    JavaThread::get()->getJVM()->illegalMonitorStateException(this);
  } 
}
