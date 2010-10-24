//===--------- ObjectLocks.cpp - Object-based locks -----------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/ObjectLocks.h"
#include "mvm/Threads/Thread.h"
#include "mvm/VirtualMachine.h"
#include "MvmGC.h"
#include "cterror.h"
#include <cerrno>
#include <sys/time.h>
#include <pthread.h>


using namespace mvm;

void ThinLock::overflowThinLock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  FatLock* obj = table.allocate(object);
  obj->acquireAll(object, (ThinCountMask >> ThinCountShift) + 1);
  uintptr_t oldValue = 0;
  uintptr_t newValue = 0;
  uintptr_t yieldedValue = 0;
  do {
    oldValue = object->header;
    newValue = obj->getID() | (oldValue & NonLockBitsMask);
    yieldedValue = __sync_val_compare_and_swap(&(object->header), oldValue, newValue);
  } while (yieldedValue != oldValue);
}
 
/// initialise - Initialise the value of the lock.
///
void ThinLock::initialise(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  uintptr_t oldValue = 0;
  uintptr_t newValue = 0;
  uintptr_t yieldedValue = 0;
  do {
    oldValue = object->header;
    newValue = oldValue & NonLockBitsMask;
    yieldedValue = __sync_val_compare_and_swap(&object->header, oldValue, newValue);
  } while (yieldedValue != oldValue);
}
  
FatLock* ThinLock::changeToFatlock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (!(object->header & FatMask)) {
    FatLock* obj = table.allocate(object);
    uint32 count = (object->header & ThinCountMask) >> ThinCountShift;
    obj->acquireAll(object, count + 1);
    uintptr_t oldValue = 0;
    uintptr_t newValue = 0;
    uintptr_t yieldedValue = 0;
    do {
      oldValue = object->header;
      newValue = obj->getID() | (oldValue & NonLockBitsMask);
      yieldedValue = __sync_val_compare_and_swap(&(object->header), oldValue, newValue);
    } while (yieldedValue != oldValue);
    return obj;
  } else {
    FatLock* res = table.getFatLockFromID(object->header);
    assert(res && "Lock deallocated while held.");
    return res;
  }
}

void ThinLock::acquire(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  uint64_t id = mvm::Thread::get()->getThreadID();
  uintptr_t oldValue = 0;
  uintptr_t newValue = 0;
  uintptr_t yieldedValue = 0;
  do {
    oldValue = object->header & NonLockBitsMask;
    newValue = oldValue | id;
    yieldedValue = __sync_val_compare_and_swap(&(object->header), oldValue, newValue);
  } while ((object->header & ~NonLockBitsMask) == 0);

  if (yieldedValue == oldValue) {
    assert(owner(object, table) && "Not owner after quitting acquire!");
    return;
  }
  
  if ((yieldedValue & Thread::IDMask) == id) {
    assert(owner(object, table) && "Inconsistent lock");
    if ((yieldedValue & ThinCountMask) != ThinCountMask) {
      do {
        oldValue = object->header;
        newValue = oldValue + ThinCountAdd;
        yieldedValue = __sync_val_compare_and_swap(&(object->header), oldValue, newValue);
      } while (oldValue != yieldedValue);
    } else {
      overflowThinLock(object, table);
    }
    assert(owner(object, table) && "Not owner after quitting acquire!");
    return;
  }
      
  while (true) {
    if (object->header & FatMask) {
      FatLock* obj = table.getFatLockFromID(object->header);
      if (obj != NULL) {
        if (obj->acquire(object)) {
          break;
        }
      }
    }
    
    while (object->header & ~NonLockBitsMask) {
      if (object->header & FatMask) {
        break;
      } else {
        mvm::Thread::yield();
      }
    }
    
    if ((object->header & ~NonLockBitsMask) == 0) {
      FatLock* obj = table.allocate(object);
      do {
        oldValue = object->header & NonLockBitsMask;
        newValue = oldValue | obj->getID();
        yieldedValue = __sync_val_compare_and_swap(&object->header, oldValue, newValue);
      } while ((object->header & ~NonLockBitsMask) == 0);

      if (oldValue != yieldedValue) {
        assert((getFatLock(object, table) != obj) && "Inconsistent lock");
        table.deallocate(obj);
      } else {
        assert((getFatLock(object, table) == obj) && "Inconsistent lock");
      }
      assert(!owner(object, table) && "Inconsistent lock");
    }
  }

  assert(owner(object, table) && "Not owner after quitting acquire!");
}

/// release - Release the lock.
void ThinLock::release(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  assert(owner(object, table) && "Not owner when entering release!");
  uint64 id = mvm::Thread::get()->getThreadID();
  uintptr_t oldValue = 0;
  uintptr_t newValue = 0;
  uintptr_t yieldedValue = 0;
  if ((object->header & ~NonLockBitsMask) == id) {
    do {
      oldValue = object->header;
      newValue = oldValue & NonLockBitsMask;
      yieldedValue = __sync_val_compare_and_swap(&object->header, oldValue, newValue);
    } while (yieldedValue != oldValue);
  } else if (object->header & FatMask) {
    FatLock* obj = table.getFatLockFromID(object->header);
    assert(obj && "Lock deallocated while held.");
    obj->release(object, table);
  } else {
    assert(((object->header & ThinCountMask) > 0) && "Inconsistent state");    
    do {
      oldValue = object->header;
      newValue = oldValue - ThinCountAdd;
      yieldedValue = __sync_val_compare_and_swap(&(object->header), oldValue, newValue);
    } while (yieldedValue != oldValue);
  }
}

/// owner - Returns true if the curren thread is the owner of this object's
/// lock.
bool ThinLock::owner(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (object->header & FatMask) {
    FatLock* obj = table.getFatLockFromID(object->header);
    if (obj != NULL) return obj->owner();
  } else {
    uint64 id = mvm::Thread::get()->getThreadID();
    if ((object->header & Thread::IDMask) == id) return true;
  }
  return false;
}

/// getFatLock - Get the fat lock is the lock is a fat lock, 0 otherwise.
FatLock* ThinLock::getFatLock(gc* object, LockSystem& table) {
  llvm_gcroot(object, 0);
  if (object->header & FatMask) {
    return table.getFatLockFromID(object->header);
  } else {
    return NULL;
  }
}

void FatLock::acquireAll(gc* obj, uint32 nb) {
  internalLock.lockAll(nb);
}

bool FatLock::owner() {
  return internalLock.selfOwner();
}
 
mvm::Thread* FatLock::getOwner() {
  return internalLock.getOwner();
}
  
FatLock::FatLock(uint32_t i, gc* a) {
  llvm_gcroot(a, 0);
  firstThread = NULL;
  index = i;
  associatedObject = a;
  waitingThreads = 0;
  lockingThreads = 0;
  nextFreeLock = NULL;
}

uintptr_t FatLock::getID() {
  return (index << mvm::NonLockBits) | mvm::FatMask;
}

void FatLock::release(gc* obj, LockSystem& table) {
  llvm_gcroot(obj, 0);
  assert(associatedObject && "No associated object when releasing");
  assert(associatedObject == obj && "Mismatch object in lock");
  if (!waitingThreads && !lockingThreads &&
      internalLock.recursionCount() == 1) {
    mvm::ThinLock::initialise(associatedObject, table);
    table.deallocate(this);
  }
  internalLock.unlock();
}

/// acquire - Acquires the internalLock.
///
bool FatLock::acquire(gc* obj) {
  llvm_gcroot(obj, 0);
    
  spinLock.lock();
  lockingThreads++;
  spinLock.unlock();
    
  internalLock.lock();
    
  spinLock.lock();
  lockingThreads--;
  spinLock.unlock();

  if (associatedObject != obj) {
    internalLock.unlock();
    return false;
  }
  return true;
}


void LockSystem::deallocate(FatLock* lock) {
  lock->associatedObject = NULL;
  threadLock.lock();
  lock->nextFreeLock = freeLock;
  freeLock = lock;
  threadLock.unlock();
}
  
LockSystem::LockSystem(mvm::BumpPtrAllocator& all) : allocator(all) {
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
    res->associatedObject = obj;
  } else { 
    // Get an index.
    uint32_t index = currentIndex++;
    if (index == MaxLocks) {
      fprintf(stderr, "Ran out of space for allocating locks");
      abort();
    }
  
    FatLock** tab = LockTable[index >> BitIndex];
  
    VirtualMachine* vm = mvm::Thread::get()->MyVM;
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
   
  // Return the lock.
  return res;
}


FatLock* LockSystem::getFatLockFromID(uintptr_t ID) {
  if (ID & mvm::FatMask) {
    uint32_t index = (ID & ~mvm::FatMask) >> mvm::NonLockBits;
    FatLock* res = getLock(index);
    return res;
  } else {
    return NULL;
  }
}



bool LockingThread::wait(
    gc* self, LockSystem& table, struct timeval* info, bool timed) {
  llvm_gcroot(self, 0);
  FatLock* l = 0;

  assert(mvm::ThinLock::owner(self, table));
    l = mvm::ThinLock::changeToFatlock(self, table);
    this->waitsOn = l;
    mvm::Cond& varcondThread = this->varcond;

    if (this->interruptFlag != 0) {
      this->interruptFlag = 0;
      this->waitsOn = 0;
      return true;
    } else { 
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
          assert(!this->prevWaiting && "Inconstitent state");
          // Notify lost, notify someone else.
          notify(self, table);
        }
      } else {
        assert(!this->prevWaiting && !this->nextWaiting &&
               "Inconsistent state");
      }
      
      this->state = LockingThread::StateRunning;
      this->waitsOn = 0;

      if (interrupted) {
        this->interruptFlag = 0;
        return true;
      }
    }
  
  assert(mvm::ThinLock::owner(self, table) && "Not owner after wait");
  return false;
}

void LockingThread::notify(gc* self, LockSystem& table) {
  llvm_gcroot(self, 0);
  assert(mvm::ThinLock::owner(self, table));
  FatLock* l = mvm::ThinLock::getFatLock(self, table);
    if (l) {
      LockingThread* cur = l->firstThread;
      if (cur) {
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

  assert(mvm::ThinLock::owner(self, table) && "Not owner after notify");
}

void LockingThread::notifyAll(gc* self, LockSystem& table) {
  llvm_gcroot(self, 0);
  assert(mvm::ThinLock::owner(self, table));
  FatLock* l = mvm::ThinLock::getFatLock(self, table);
    if (l) {
      LockingThread* cur = l->firstThread;
      if (cur) {
        do {
          LockingThread* temp = cur->nextWaiting;
          cur->prevWaiting = 0;
          cur->nextWaiting = 0;
          cur->varcond.signal();
          cur = temp;
        } while (cur != l->firstThread);
        l->firstThread = 0;
      }
    }

  assert(mvm::ThinLock::owner(self, table) && "Not owner after notifyAll");
}
