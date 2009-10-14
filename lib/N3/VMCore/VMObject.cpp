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
														(uintptr_t)mvm::Object::default_hashCode);

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
	hashCode = h;
}

uint32 N3VirtualTable::baseVtSize() {
	return sizeof(N3VirtualTable) / sizeof(uintptr_t);
}


LockObj* LockObj::allocate() {
  declare_gcroot(LockObj*, res) = new(&_VT) LockObj();
	res->threads = new std::vector<VMThread*>();
	res->lock    = new mvm::LockRecursive();
  return res;
}

void LockObj::_print(const LockObj *self, mvm::PrintBuffer* buf) {
	llvm_gcroot(self, 0);
  buf->write("Lock<>");
}

void LockObj::_destroy(LockObj *self) {
	llvm_gcroot(self, 0);
	delete self->threads;
	delete self->lock;
}

void LockObj::notify(LockObj *self) {
	llvm_gcroot(self, 0);
  for (std::vector<VMThread*>::iterator i = self->threads->begin(), 
				 e = self->threads->end(); i!= e; ++i) {
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
				self->threads->erase(i);
				break;
			} else { // dead thread
				self->threads->erase(i);
			}
		}
  }
}

void LockObj::notifyAll(LockObj *self) {
	llvm_gcroot(self, 0);
  for (std::vector<VMThread*>::iterator i = self->threads->begin(),
				 e = self->threads->end(); i!= e; ++i) {
    VMThread* cur = *i;
    cur->lock->lock();
    cur->varcond->signal();
    cur->lock->unlock();
    self->threads->erase(i);
  }
}

void LockObj::wait(LockObj *self, VMThread* th) {
	llvm_gcroot(self, 0);
  self->threads->push_back(th);
}

void LockObj::remove(LockObj *self, VMThread* th) {
	llvm_gcroot(self, 0);
  for (std::vector<VMThread*>::iterator i = self->threads->begin(),
				 e = self->threads->end(); i!= e; ++i) {
    if (*i == th) {
      self->threads->erase(i);
      break;
    }
  }
}

void LockObj::aquire(LockObj *self) {
	llvm_gcroot(self, 0);
  self->lock->lock();
}

void LockObj::release(LockObj *self) {
	llvm_gcroot(self, 0);
  self->lock->unlock();
}

bool LockObj::owner(LockObj *self) {
	llvm_gcroot(self, 0);
  return self->lock->selfOwner();
}

static LockObj* myLock(VMObject* obj) {
	llvm_gcroot(obj, 0);
  verifyNull(obj);
	declare_gcroot(LockObj*, lock) = obj->lockObj;
  if (lock == 0) {
    VMObject::globalLock->lock();
		lock = obj->lockObj;
    if (lock == 0) {
			lock = LockObj::allocate();
      obj->lockObj = lock;
    }
    VMObject::globalLock->unlock();
  }
  return lock;
}

void VMObject::initialise(VMObject* self, VMCommonClass* cl) {
	llvm_gcroot(self, 0);
  self->classOf = cl;
  self->lockObj = 0;
}

void VMObject::_print(const VMObject *self, mvm::PrintBuffer* buf) {
	llvm_gcroot(self, 0);
  buf->write("VMObject<");
  self->classOf->print(buf);
	buf->write("@0x");
	buf->writePtr((void*)self->hashCode());
  buf->write(">");
}

void VMObject::aquire(VMObject* self) {
	llvm_gcroot(self, 0);
	declare_gcroot(LockObj*, lock) = myLock(self);
	LockObj::aquire(lock);
}

void VMObject::unlock(VMObject* self) {
	llvm_gcroot(self, 0);
  verifyNull(self);
	declare_gcroot(LockObj*, lock) = myLock(self);
	LockObj::release(lock);
}

void VMObject::waitIntern(VMObject* self, struct timeval* info, bool timed) {
	llvm_gcroot(self, 0);
  declare_gcroot(LockObj *, l) = myLock(self);
  bool owner = LockObj::owner(l);

  if (owner) {
    VMThread* thread = VMThread::get();
    mvm::Lock* mutexThread = thread->lock;
    mvm::Cond* varcondThread = thread->varcond;

    mutexThread->lock();
    if (thread->interruptFlag != 0) {
      mutexThread->unlock();
      thread->interruptFlag = 0;
      thread->getVM()->interruptedException(self);
    } else {
      unsigned int recur = l->lock->recursionCount();
      bool timeout = false;
      l->lock->unlockAll();
			LockObj::wait(l, thread);
      thread->state = VMThread::StateWaiting;

      if (timed) {
        timeout = varcondThread->timedWait(mutexThread, info);
      } else {
        varcondThread->wait(mutexThread);
      }

      bool interrupted = (thread->interruptFlag != 0);
      mutexThread->unlock();
      l->lock->lockAll(recur);

      if (interrupted || timeout) {
				LockObj::remove(l, thread);
      }

      thread->state = VMThread::StateRunning;

      if (interrupted) {
        thread->interruptFlag = 0;
        thread->getVM()->interruptedException(self);
      }
    }
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(self);
  }
}

void VMObject::wait(VMObject* self) {
	llvm_gcroot(self, 0);
  waitIntern(self, 0, false);
}

void VMObject::timedWait(VMObject* self, struct timeval& info) {
	llvm_gcroot(self, 0);
  waitIntern(self, &info, false);
}

void VMObject::notify(VMObject* self) {
	llvm_gcroot(self, 0);
  declare_gcroot(LockObj*, l) = myLock(self);
  if (LockObj::owner(l)) {
		LockObj::notify(l);
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(self);
  }
}

void VMObject::notifyAll(VMObject* self) {
	llvm_gcroot(self, 0);
  declare_gcroot(LockObj*, l) = myLock(self);
  if (LockObj::owner(l)) {
		LockObj::notifyAll(l);
  } else {
    VMThread::get()->getVM()->illegalMonitorStateException(self);
  } 
}

bool VMObject::instanceOf(VMObject* self, VMCommonClass* cl) {
	llvm_gcroot(self, 0);
  if (!self) return false;
  else return self->classOf->isAssignableFrom(cl);
}
