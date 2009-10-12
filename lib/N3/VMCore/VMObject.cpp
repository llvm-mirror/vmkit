//===------------- VMObject.cpp - VM object definition --------------------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/Threads/Locks.h"

#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"
#include "N3.h"

using namespace n3;

void VMObject::initialise(VMCommonClass* cl) {
  this->classOf = cl;
  this->lockObj = 0;
}

void VMCond::notify() {
  for (std::vector<VMThread*>::iterator i = threads.begin(), 
            e = threads.end(); i!= e; ++i) {
    VMThread* cur = *i;
    cur->lock->lock();
    if (cur->interruptFlag != 0) {
      cur->lock->unlock();
      continue;
    } else {
			declare_gcroot(VMObject *, th) = cur->vmThread;
			if (th != 0) {
				cur->varcond->signal();
				cur->lock->unlock();
				threads.erase(i);
				break;
			} else { // dead thread
				threads.erase(i);
			}
		}
  }
}

void VMCond::notifyAll() {
  for (std::vector<VMThread*>::iterator i = threads.begin(),
            e = threads.end(); i!= e; ++i) {
    VMThread* cur = *i;
    cur->lock->lock();
    cur->varcond->signal();
    cur->lock->unlock();
    threads.erase(i);
  }
}

void VMCond::wait(VMThread* th) {
  threads.push_back(th);
}

void VMCond::remove(VMThread* th) {
  for (std::vector<VMThread*>::iterator i = threads.begin(),
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
  return res;
}

void LockObj::aquire() {
  lock.lock();
}

void LockObj::release() {
  lock.unlock();
}

bool LockObj::owner() {
  return lock.selfOwner();
}

void VMObject::print(mvm::PrintBuffer* buf) const {
  buf->write("VMObject<");
  classOf->print(buf);
  buf->write(">");
}

VMObject* VMObject::allocate(VMCommonClass* cl) {
  VMObject* res = gc_new(VMObject)();
  res->classOf = cl;
  return res;
}

static LockObj* myLock(VMObject* obj) {
  verifyNull(obj);
  if (obj->lockObj == 0) {
    VMObject::globalLock->lock();
    if (obj->lockObj == 0) {
      obj->lockObj = LockObj::allocate();
    }
    VMObject::globalLock->unlock();
  }
  return obj->lockObj;
}

void VMObject::aquire() {
  myLock(this)->aquire();
}

void VMObject::unlock() {
  verifyNull(this);
  lockObj->release();
}

void VMObject::waitIntern(struct timeval* info, bool timed) {
  LockObj * l = myLock(this);
  bool owner = l->owner();

  if (owner) {
    VMThread* thread = VMThread::get();
    mvm::Lock* mutexThread = thread->lock;
    mvm::Cond* varcondThread = thread->varcond;

    mutexThread->lock();
    if (thread->interruptFlag != 0) {
      mutexThread->unlock();
      thread->interruptFlag = 0;
      thread->vm->interruptedException(this);
    } else {
      unsigned int recur = l->lock.recursionCount();
      bool timeout = false;
      l->lock.unlockAll();
      l->varcond.wait(thread);
      thread->state = VMThread::StateWaiting;

      if (timed) {
        timeout = varcondThread->timedWait(mutexThread, info);
      } else {
        varcondThread->wait(mutexThread);
      }

      bool interrupted = (thread->interruptFlag != 0);
      mutexThread->unlock();
      l->lock.lockAll(recur);

      if (interrupted || timeout) {
        l->varcond.remove(thread);
      }

      thread->state = VMThread::StateRunning;

      if (interrupted) {
        thread->interruptFlag = 0;
        thread->vm->interruptedException(this);
      }
    }
  } else {
    VMThread::get()->vm->illegalMonitorStateException(this);
  }
}

void VMObject::wait() {
  waitIntern(0, false);
}

void VMObject::timedWait(struct timeval& info) {
  waitIntern(&info, false);
}

void VMObject::notify() {
  LockObj* l = myLock(this);
  if (l->owner()) {
    l->varcond.notify();
  } else {
    VMThread::get()->vm->illegalMonitorStateException(this);
  }
}

void VMObject::notifyAll() {
  LockObj* l = myLock(this);
  if (l->owner()) {
    l->varcond.notifyAll();
  } else {
    VMThread::get()->vm->illegalMonitorStateException(this);
  } 
}

bool VMObject::instanceOf(VMCommonClass* cl) {
  if (!this) return false;
  else return this->classOf->isAssignableFrom(cl);
}
