//===--------- ObjectLocks.cpp - Object-based locks -----------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>

#include "vmkit/Cond.h"
#include "vmkit/Locks.h"
#include "vmkit/ObjectLocks.h"
#include "vmkit/Thread.h"
#include "vmkit/VirtualMachine.h"
#include "VmkitGC.h"
#include <cerrno>
#include <sys/time.h>
#include <pthread.h>

#include "../../j3/VMCore/JavaObject.h"
#include "../../j3/VMCore/JavaClass.h"
#include "../../j3/VMCore/UTF8.h"

namespace vmkit {

void ThinLock::overflowThinLock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  FatLock* obj = table.allocate(object);
  word_t ID = obj->getID();
  // 1 because we start at 0, and 1 for this lock request.
  obj->acquireAll(object, (ThinCountMask >> ThinCountShift) + 2);
  word_t oldValue = 0;
  word_t newValue = 0;
  word_t yieldedValue = 0;
  do {
    oldValue = object->header();
    newValue = obj->getID() | (oldValue & NonLockBitsMask);
    assert(obj->associatedObject == object);
    yieldedValue = __sync_val_compare_and_swap(&(object->header()), oldValue, newValue);
  } while (((object->header()) & ~NonLockBitsMask) != ID);
  assert(obj->associatedObject == object);
}
 
/// initialise - Initialise the value of the lock.
///
#if 0
// Disable fat lock deflation code in ObjectLocks.cpp
// Fixes occasional assertion failure when running on multi-core machine.
void ThinLock::removeFatLock(FatLock* fatLock, LockSystem& table) {
  gc* object = fatLock->associatedObject;
  llvm_gcroot(object, 0);
  word_t ID;
  word_t oldValue = 0;
  word_t newValue = 0;
  word_t yieldedValue = 0;

  ID = fatLock->getID();
  do {
    oldValue = object->header();
    newValue = oldValue & NonLockBitsMask;
    yieldedValue = __sync_val_compare_and_swap(&object->header(), oldValue, newValue);
  } while (oldValue != yieldedValue);
  assert((oldValue & NonLockBitsMask) != ID);
  fatLock->associatedObject = NULL;
}
#endif
  
FatLock* ThinLock::changeToFatlock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (!(object->header() & FatMask)) {
    FatLock* obj = table.allocate(object);
    uint32 count = (object->header() & ThinCountMask) >> ThinCountShift;
    obj->acquireAll(object, count + 1);
    word_t oldValue = 0;
    word_t newValue = 0;
    word_t yieldedValue = 0;
    word_t ID = obj->getID();
    do {
      oldValue = object->header();
      newValue = ID | (oldValue & NonLockBitsMask);
      assert(obj->associatedObject == object);
      yieldedValue = __sync_val_compare_and_swap(&(object->header()), oldValue, newValue);
    } while (((object->header()) & ~NonLockBitsMask) != ID);
    return obj;
  } else {
    FatLock* res = table.getFatLockFromID(object->header());
    assert(res && "Lock deallocated while held.");
    assert(res->associatedObject == object);
    return res;
  }
}

void printDebugMessage(gc* object, LockSystem& table) {
  gc* assObj = NULL;
  llvm_gcroot(assObj, 0);
  llvm_gcroot(object, 0);
  fprintf(stderr,
      "WARNING: [%p] has been waiting really long for %p (header = %p)\n",
      (void*)vmkit::Thread::get(),
      (void*)object,
      (void*)object->header());
  FatLock* obj = table.getFatLockFromID(object->header());
  if (obj != NULL) {
    fprintf(stderr,
        "WARNING: [%p] is waiting on fatlock %p. "
        "Its associated object is %p. The owner is %p\n",
        (void*)vmkit::Thread::get(),
        (void*)obj,
        (void*)(assObj = obj->getAssociatedObject()),
        (void*)obj->owner());
  }
}

void ThinLock::acquire(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  uint64_t id = vmkit::Thread::get()->getThreadID();
  word_t oldValue = 0;
  word_t newValue = 0;
  word_t yieldedValue = 0;

  if ((object->header() & System::GetThreadIDMask()) == id) {
    assert(owner(object, table) && "Inconsistent lock");
    if ((object->header() & ThinCountMask) != ThinCountMask) {
      uint32 count = object->header() & ThinCountMask;
      do {
        oldValue = object->header();
        newValue = oldValue + ThinCountAdd;
        yieldedValue = __sync_val_compare_and_swap(&(object->header()), oldValue, newValue);
      } while ((object->header() & ThinCountMask) == count);
    } else {
      overflowThinLock(object, table);
    }
    assert(owner(object, table) && "Not owner after quitting acquire!");
    return;
  }

  do {
    oldValue = object->header() & NonLockBitsMask;
    newValue = oldValue | id;
    yieldedValue = __sync_val_compare_and_swap(&(object->header()), oldValue, newValue);
  } while ((object->header() & ~NonLockBitsMask) == 0);

  if (((object->header()) & ~NonLockBitsMask) == id) {
    assert(owner(object, table) && "Not owner after quitting acquire!");
    return;
  }

  // Simple counter to lively diagnose possible dead locks in this code.
  int counter = 0;  
  while (true) {
    if (object->header() & FatMask) {
      FatLock* obj = table.getFatLockFromID(object->header());
      if (obj != NULL) {
        if (obj->acquire(object, table)) {
          assert((object->header() & FatMask) && "Inconsistent lock");
          assert((table.getFatLockFromID(object->header()) == obj) && "Inconsistent lock");
          assert(owner(object, table) && "Not owner after acquiring fat lock!");
          break;
        }
      }
    }
   
    counter++;
    if (counter == 1000) printDebugMessage(object, table);

    while (object->header() & ~NonLockBitsMask) {
      if (object->header() & FatMask) {
        break;
      } else {
        vmkit::Thread::yield();
      }
    }
    
    if ((object->header() & ~NonLockBitsMask) == 0) {
      FatLock* obj = table.allocate(object);
      obj->internalLock.lock();
      do {
        oldValue = object->header() & NonLockBitsMask;
        newValue = oldValue | obj->getID();
        assert(obj->associatedObject == object);
        yieldedValue = __sync_val_compare_and_swap(&object->header(), oldValue, newValue);
      } while ((object->header() & ~NonLockBitsMask) == 0);

      if ((getFatLock(object, table) != obj)) {
        assert((object->header() & ~NonLockBitsMask) != obj->getID());
        obj->internalLock.unlock();
        table.deallocate(obj);
      } else {
        assert((object->header() & ~NonLockBitsMask) == obj->getID());
        assert(owner(object, table) && "Inconsistent lock");
        break;
      }
    }
  }

  assert(owner(object, table) && "Not owner after quitting acquire!");
}

/// release - Release the lock.
void ThinLock::release(gc* object, LockSystem& table, vmkit::Thread* ownerThread) {
  llvm_gcroot(object, 0);
  if (!ownerThread) ownerThread = vmkit::Thread::get();
  assert((getOwner(object, table) == ownerThread) && "Not owner when entering release!");
  uint64 id = ownerThread->getThreadID();
  word_t oldValue = 0;
  word_t newValue = 0;
  word_t yieldedValue = 0;

  if ((object->header() & ~NonLockBitsMask) == id) {
    do {
      oldValue = object->header();
      newValue = oldValue & NonLockBitsMask;
      yieldedValue = __sync_val_compare_and_swap(&object->header(), oldValue, newValue);
    } while ((object->header() & ~NonLockBitsMask) == id);
  } else if (object->header() & FatMask) {
    FatLock* obj = table.getFatLockFromID(object->header());
    assert(obj && "Lock deallocated while held.");
    obj->release(object, table, ownerThread);
  } else {
    assert(((object->header() & ThinCountMask) > 0) && "Inconsistent state");
    uint32 count = (object->header() & ThinCountMask);
    do {
      oldValue = object->header();
      newValue = oldValue - ThinCountAdd;
      yieldedValue = __sync_val_compare_and_swap(&(object->header()), oldValue, newValue);
    } while ((object->header() & ThinCountMask) == count);
  }

}

/// owner - Returns true if the current thread is the owner of this object's
/// lock.
bool ThinLock::owner(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (object->header() & FatMask) {
    FatLock* obj = table.getFatLockFromID(object->header());
    if (obj != NULL) return obj->owner();
  } else {
  	bool res = false;
    uint64 id = vmkit::Thread::get()->getThreadID();
    res = ((object->header() & System::GetThreadIDMask()) == id);
    if (res) return true;
  }
  return false;
}

vmkit::Thread* ThinLock::getOwner(gc* object, LockSystem& table)
{
  llvm_gcroot(object, 0);
  if (object->header() & FatMask) {
	FatLock* obj = table.getFatLockFromID(object->header());
	return (!obj) ? NULL : obj->getOwner();
  } else {
	uint64_t threadID = object->header() & System::GetThreadIDMask();
	return vmkit::Thread::getByID(threadID);
  }
}

/// getFatLock - Get the fat lock is the lock is a fat lock, 0 otherwise.
FatLock* ThinLock::getFatLock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (object->header() & FatMask) {
    return table.getFatLockFromID(object->header());
  } else {
    return NULL;
  }
}

void FatLock::acquireAll(gc* object, word_t nb) {
  llvm_gcroot(object, 0);
  assert(associatedObject == object);
  internalLock.lockAll(nb);
}

bool FatLock::owner() {
  return internalLock.selfOwner();
}
 
vmkit::Thread* FatLock::getOwner() {
  return internalLock.getOwner();
}
  
FatLock::FatLock(uint32_t i, gc* a) :
	associatedObjectDead(false)
{
  llvm_gcroot(a, 0);
  assert(a != NULL);
  firstThread = NULL;
  index = i;
  setAssociatedObject(a);
  waitingThreads = 0;
  lockingThreads = 0;
  nextFreeLock = NULL;
}

word_t FatLock::getID() {
  return (index << ThinLock::NonLockBits) | ThinLock::FatMask;
}

void FatLock::release(gc* obj, LockSystem& table, vmkit::Thread* ownerThread) {
  llvm_gcroot(obj, 0);
  assert(associatedObject && "No associated object when releasing");
  assert(associatedObject == obj && "Mismatch object in lock");
#if 0
  // Disable fat lock deflation code in ObjectLocks.cpp
  // Fixes occasional assertion failure when running on multi-core machine.
  if (!waitingThreads && !lockingThreads &&
      internalLock.recursionCount() == 1) {
    vmkit::ThinLock::removeFatLock(this, table);
    table.deallocate(this);
  }
#endif
  internalLock.unlock(ownerThread);
}

/// acquire - Acquires the internalLock.
///
bool FatLock::acquire(gc* obj, LockSystem& table) {
  llvm_gcroot(obj, 0);
    
  spinLock.lock();
  lockingThreads++;
  spinLock.unlock();
    
  internalLock.lock();
    
  spinLock.lock();
  lockingThreads--;
  spinLock.unlock();

  if (this->associatedObjectIsDead()) {
    internalLock.unlock();

//	if (lockingThreads == 0)
//      table.deallocate(this);

	word_t methodIP = System::GetCallFrameAddress();
	methodIP = System::GetReturnAddressOfCallFrame(methodIP);
    Thread::get()->throwNullPointerException(methodIP);
  }

  if (associatedObject != obj) {
    internalLock.unlock();
    return false;
  }
  assert(obj->header() & ThinLock::FatMask);
  assert((obj->header() & ~ThinLock::NonLockBitsMask) == getID());
  return true;
}


void LockSystem::deallocate(FatLock* lock) {
  lock->associatedObject = NULL;
  threadLock.lock();
  lock->nextFreeLock = freeLock;
  freeLock = lock;
  threadLock.unlock();
}
  
LockSystem::LockSystem(vmkit::BumpPtrAllocator& all) : allocator(all) {
  assert(ThinLock::ThinCountMask > 0);
  LockTable = (FatLock* **)
    allocator.Allocate(GlobalSize * sizeof(FatLock**), "Global LockTable");
  LockTable[0] = (FatLock**)
    allocator.Allocate(IndexSize * sizeof(FatLock*), "Index LockTable");
  currentIndex = 0;
  freeLock = NULL;
}

FatLock* LockSystem::allocate(gc* obj) {  
  llvm_gcroot(obj, 0); 
  FatLock* res = 0;
  threadLock.lock();

  // Try the freeLock list.
  if (freeLock != NULL) {
    res = freeLock;
    freeLock = res->nextFreeLock;
    res->nextFreeLock = 0;
    assert(res->associatedObject == NULL);
    threadLock.unlock();
    res->setAssociatedObject(obj);
  } else { 
    // Get an index.
    uint32_t index = currentIndex++;
    if (index == MaxLocks) {
      fprintf(stderr, "Ran out of space for allocating locks");
      abort();
    }
  
    FatLock** &tab = LockTable[index >> BitIndex];
  
    VirtualMachine* vm = vmkit::Thread::get()->MyVM;
    if (tab == NULL) {
      tab = (FatLock**)vm->allocator.Allocate(
          IndexSize * sizeof(FatLock*), "Index LockTable");
    }
    threadLock.unlock();
   
    // Allocate the lock.
    res = new(vm->allocator, "Lock") FatLock(index, obj);
    
    // Add the lock to the table.
    uint32_t internalIndex = index & BitMask;
    tab[internalIndex] = res;
  }
   
  assert(res->associatedObject == obj);
  // Return the lock.
  return res;
}


FatLock* LockSystem::getFatLockFromID(word_t ID) {
  if (ID & ThinLock::FatMask) {
    uint32_t index = (ID & ~ThinLock::FatMask) >> ThinLock::NonLockBits;
    FatLock* res = getLock(index);
    return res;
  } else {
    return NULL;
  }
}



bool LockingThread::wait(
    gc* self, LockSystem& table, struct timeval* info, bool timed) {
  llvm_gcroot(self, 0);
  assert(vmkit::ThinLock::owner(self, table));

  FatLock* l = vmkit::ThinLock::changeToFatlock(self, table);
  this->waitsOn = l;
  vmkit::Cond& varcondThread = this->varcond;

  if (this->interruptFlag != 0) {
    this->interruptFlag = 0;
    this->waitsOn = 0;
    return true;
  }
  
  this->state = LockingThread::StateWaiting;

  if (l->firstThread) {
    assert(l->firstThread->prevWaiting && l->firstThread->nextWaiting &&
           "Inconsistent list");
    if (l->firstThread->nextWaiting == l->firstThread) {
      l->firstThread->nextWaiting = this;
    } else {
      l->firstThread->prevWaiting->nextWaiting = this;
    } 
    this->prevWaiting = l->firstThread->prevWaiting;
    this->nextWaiting = l->firstThread;
    l->firstThread->prevWaiting = this;
  } else {
    l->firstThread = this;
    this->nextWaiting = this;
    this->prevWaiting = this;
  }
  
  assert(this->prevWaiting && this->nextWaiting && "Inconsistent list");
  assert(l->firstThread->prevWaiting && l->firstThread->nextWaiting &&
         "Inconsistent list");
      
  bool timeout = false;

  l->waitingThreads++;

  while (!this->interruptFlag && this->nextWaiting) {
    if (timed) {
      timeout = varcondThread.timedWait(&l->internalLock, info);
      if (timeout) break;
    } else {
      varcondThread.wait(&l->internalLock);
    }
  }
  assert(vmkit::ThinLock::owner(self, table) && "Not owner after wait");
      
  l->waitingThreads--;
     
  assert((!l->firstThread || (l->firstThread->prevWaiting && 
         l->firstThread->nextWaiting)) && "Inconsistent list");
 
  bool interrupted = (this->interruptFlag != 0);

  if (interrupted || timeout) {
    if (this->nextWaiting) {
      assert(this->prevWaiting && "Inconsistent list");
      if (l->firstThread != this) {
        this->nextWaiting->prevWaiting = this->prevWaiting;
        this->prevWaiting->nextWaiting = this->nextWaiting;
        assert(l->firstThread->prevWaiting && 
               l->firstThread->nextWaiting && "Inconsistent list");
      } else if (this->nextWaiting == this) {
        l->firstThread = NULL;
      } else {
        l->firstThread = this->nextWaiting;
        l->firstThread->prevWaiting = this->prevWaiting;
        this->prevWaiting->nextWaiting = l->firstThread;
        assert(l->firstThread->prevWaiting && 
               l->firstThread->nextWaiting && "Inconsistent list");
      }
      this->nextWaiting = NULL;
      this->prevWaiting = NULL;
    } else {
      assert(!this->prevWaiting && "Inconsistent state");
      // Notify lost, notify someone else.
      notify(self, table);
    }
  } else {
    assert(!this->prevWaiting && !this->nextWaiting &&
           "Inconsistent state");
  }
      
  this->state = LockingThread::StateRunning;
  this->waitsOn = NULL;

  if (interrupted) {
    this->interruptFlag = 0;
    return true;
  }
  
  assert(vmkit::ThinLock::owner(self, table) && "Not owner after wait");
  return false;
}

void LockingThread::notify(gc* self, LockSystem& table, vmkit::Thread* ownerThread) {
  llvm_gcroot(self, 0);
  if (!ownerThread) ownerThread = vmkit::Thread::get();
  assert(vmkit::ThinLock::getOwner(self, table) == ownerThread);
  FatLock* l = vmkit::ThinLock::getFatLock(self, table);
  
  if (l == NULL) return;
  LockingThread* cur = l->firstThread;
  if (cur == NULL) return;
  
  do {
    if (cur->interruptFlag != 0) {
      cur = cur->nextWaiting;
    } else {
      assert(cur->prevWaiting && cur->nextWaiting &&
             "Inconsistent list");
      if (cur != l->firstThread) {
        cur->prevWaiting->nextWaiting = cur->nextWaiting;
        cur->nextWaiting->prevWaiting = cur->prevWaiting;
        assert(l->firstThread->prevWaiting &&
               l->firstThread->nextWaiting && "Inconsistent list");
      } else if (cur->nextWaiting == cur) {
        l->firstThread = NULL;
      } else {
        l->firstThread = cur->nextWaiting;
        l->firstThread->prevWaiting = cur->prevWaiting;
        cur->prevWaiting->nextWaiting = l->firstThread;
        assert(l->firstThread->prevWaiting && 
               l->firstThread->nextWaiting && "Inconsistent list");
      }
      cur->prevWaiting = NULL;
      cur->nextWaiting = NULL;
      cur->varcond.signal();
      break;
    }
  } while (cur != l->firstThread);

  assert((vmkit::ThinLock::getOwner(self, table) == ownerThread) && "Not owner after notify");
}

void LockingThread::notifyAll(gc* self, LockSystem& table, vmkit::Thread* ownerThread) {
  llvm_gcroot(self, 0);
  if (!ownerThread) ownerThread = vmkit::Thread::get();
  assert(vmkit::ThinLock::getOwner(self, table) == ownerThread);
  FatLock* l = vmkit::ThinLock::getFatLock(self, table);
  if (l == NULL) return;
  LockingThread* cur = l->firstThread;
  if (cur == NULL) return;
  do {
    LockingThread* temp = cur->nextWaiting;
    cur->prevWaiting = NULL;
    cur->nextWaiting = NULL;
    cur->varcond.signal();
    cur = temp;
  } while (cur != l->firstThread);
  l->firstThread = NULL;
  assert((vmkit::ThinLock::getOwner(self, table) == ownerThread) && "Not owner after notifyAll");
}

void FatLock::setAssociatedObject(gc* obj) {
  llvm_gcroot(obj, 0);
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)&associatedObject, (gc*)obj);
}

}
