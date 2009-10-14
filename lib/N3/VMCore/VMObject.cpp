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

N3VirtualTable LockObj::_VT((uintptr_t)LockObj::_destroy, 
														(uintptr_t)0, 
														(uintptr_t)mvm::Object::default_tracer, 
														(uintptr_t)LockObj::_print, 
														(uintptr_t)mvm::Object::default_hashcode);
N3VirtualTable *LockObj::VT = &_VT;

void *N3VirtualTable::operator new(size_t size, mvm::BumpPtrAllocator &allocator, size_t totalVtSize) {
	//printf("Allocate N3VirtualTable with %d elements\n", totalVtSize);
	return allocator.Allocate(totalVtSize * sizeof(uintptr_t), "N3VirtualTable");
}

N3VirtualTable::N3VirtualTable() {
}

N3VirtualTable::N3VirtualTable(N3VirtualTable *baseVt, uint32 baseVtSize, uint32 totalSize) {
	memcpy(this, baseVt, baseVtSize * sizeof(uintptr_t));
}

N3VirtualTable::N3VirtualTable(uintptr_t d, uintptr_t o, uintptr_t t, uintptr_t p, uintptr_t h) : VirtualTable(d, o, t) {
	print = p;
	hashcode = h;
}

uint32 N3VirtualTable::baseVtSize() {
	return sizeof(N3VirtualTable) / sizeof(uintptr_t);
}

void VMObject::initialise(VMCommonClass* cl) {
  this->classOf = cl;
  this->lockObj = 0;
}


LockObj* LockObj::allocate() {
  LockObj* res = gc_new(LockObj)();
  return res;
}

void LockObj::_print(const LockObj *self, mvm::PrintBuffer* buf) {
	llvm_gcroot(self, 0);
  buf->write("Lock<>");
}

void LockObj::_destroy(LockObj *self) {
	llvm_gcroot(self, 0);
}

void LockObj::notify() {
  for (std::vector<VMThread*>::iterator i = threads.begin(), 
            e = threads.end(); i!= e; ++i) {
    VMThread* cur = *i;
    cur->lock->lock();
    if (cur->interruptFlag != 0) {
      cur->lock->unlock();
      continue;
    } else {
			declare_gcroot(VMObject *, th) = cur->ooo_appThread;
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

void LockObj::notifyAll() {
  for (std::vector<VMThread*>::iterator i = threads.begin(),
            e = threads.end(); i!= e; ++i) {
    VMThread* cur = *i;
    cur->lock->lock();
    cur->varcond->signal();
    cur->lock->unlock();
    threads.erase(i);
  }
}

void LockObj::wait(VMThread* th) {
  threads.push_back(th);
}

void LockObj::remove(VMThread* th) {
  for (std::vector<VMThread*>::iterator i = threads.begin(),
            e = threads.end(); i!= e; ++i) {
    if (*i == th) {
      threads.erase(i);
      break;
    }
  }
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
      thread->getVM()->interruptedException(this);
    } else {
      unsigned int recur = l->lock.recursionCount();
      bool timeout = false;
      l->lock.unlockAll();
      l->wait(thread);
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
        l->remove(thread);
      }

      thread->state = VMThread::StateRunning;

      if (interrupted) {
        thread->interruptFlag = 0;
        thread->getVM()->interruptedException(this);
      }
    }
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(this);
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
    l->notify();
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(this);
  }
}

void VMObject::notifyAll() {
  LockObj* l = myLock(this);
  if (l->owner()) {
    l->notifyAll();
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(this);
  } 
}

bool VMObject::instanceOf(VMCommonClass* cl) {
  if (!this) return false;
  else return this->classOf->isAssignableFrom(cl);
}
