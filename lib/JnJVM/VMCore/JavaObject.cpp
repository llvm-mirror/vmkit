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
#include "Jnjvm.h"

using namespace jnjvm;

JavaCond* JavaCond::allocate() {
  return gc_new(JavaCond)();
}

void JavaCond::notify() {
  for (std::vector<JavaThread*>::iterator i = threads.begin(), 
            e = threads.end(); i!= e;) {
    JavaThread* cur = *i;
    cur->lock->lock();
    if (cur->interruptFlag != 0) {
      cur->lock->unlock();
      ++i;
      continue;
    } else if (cur->javaThread != 0) {
      cur->varcond->signal();
      cur->lock->unlock();
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
    cur->lock->lock();
    cur->varcond->signal();
    cur->lock->unlock();
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

void LockObj::print(mvm::PrintBuffer* buf) const {
  buf->write("Lock<>");
}

LockObj* LockObj::allocate() {
  LockObj* res = gc_new(LockObj)();
  res->lock = mvm::Lock::allocRecursive();
  res->varcond = JavaCond::allocate();
  return res;
}

void LockObj::aquire() {
  lock->lock();
}

void LockObj::release() {
  lock->unlock();
}

bool LockObj::owner() {
  return mvm::Lock::selfOwner(lock);
}

void JavaObject::print(mvm::PrintBuffer* buf) const {
  buf->write("JavaObject<");
  CommonClass::printClassName(classOf->name, buf);
  buf->write(">");
}

JavaObject* JavaObject::allocate(CommonClass* cl) {
  JavaObject* res = gc_new(JavaObject)();
  res->classOf = cl;
  return res;
}

static LockObj* myLock(JavaObject* obj) {
  verifyNull(obj);
  if (obj->lockObj == 0) {
    JavaObject::globalLock->lock();
    if (obj->lockObj == 0) {
      obj->lockObj = LockObj::allocate();
    }
    JavaObject::globalLock->unlock();
  }
  return obj->lockObj;
}

void JavaObject::aquire() {
  myLock(this)->aquire();
}

void JavaObject::unlock() {
  verifyNull(this);
  lockObj->release();
}

void JavaObject::waitIntern(struct timeval* info, bool timed) {
  LockObj * l = myLock(this);
  bool owner = l->owner();

  if (owner) {
    JavaThread* thread = JavaThread::get();
    mvm::Lock* mutexThread = thread->lock;
    mvm::Cond* varcondThread = thread->varcond;

    mutexThread->lock();
    if (thread->interruptFlag != 0) {
      mutexThread->unlock();
      thread->interruptFlag = 0;
      thread->isolate->interruptedException(this);
    } else {
      unsigned int recur = mvm::LockRecursive::recursion_count(l->lock);
      bool timeout = false;
      mvm::LockRecursive::my_unlock_all(l->lock);
      l->varcond->wait(thread);
      thread->state = JavaThread::StateWaiting;

      if (timed) {
        timeout = varcondThread->timed_wait(mutexThread, info);
      } else {
        varcondThread->wait(mutexThread);
      }

      bool interrupted = (thread->interruptFlag != 0);
      mutexThread->unlock();
      mvm::LockRecursive::my_lock_all(l->lock, recur);

      if (interrupted || timeout) {
        l->varcond->remove(thread);
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
  LockObj* l = myLock(this);
  if (l->owner()) {
    l->varcond->notify();
  } else {
    JavaThread::get()->isolate->illegalMonitorStateException(this);
  }
}

void JavaObject::notifyAll() {
  LockObj* l = myLock(this);
  if (l->owner()) {
    l->varcond->notifyAll();
  } else {
    JavaThread::get()->isolate->illegalMonitorStateException(this);
  } 
}

bool JavaObject::instanceOfString(const UTF8* name) {
  if (!this) return false;
  else return this->classOf->isOfTypeName(name);
}

bool JavaObject::checkCast(const UTF8* Tname) {
  if (!this || this->classOf->isOfTypeName(Tname)) {
    return true;
  } else {
    JavaThread::get()->isolate->classCastException("checkcast"); 
    return false;
  }
}

bool JavaObject::instanceOf(CommonClass* cl) {
  if (!this) return false;
  else return this->classOf->isAssignableFrom(cl);
}
