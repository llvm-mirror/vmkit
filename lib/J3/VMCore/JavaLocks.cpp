//===------------- JavaLocks.cpp - Fat lock management --------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaLocks.h"
#include "JavaThread.h"
#include "Jnjvm.h"

using namespace j3;

JavaLock* JavaLock::allocate(JavaObject* obj) {
  llvm_gcroot(obj, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaLock* res = vm->lockSystem.allocate(obj);  
  return res;
}

void JavaLock::deallocate() {
  Jnjvm* vm = JavaThread::get()->getJVM();
  vm->lockSystem.deallocate(this);
}

JavaLock* LockSystem::allocate(JavaObject* obj) { 
 
  llvm_gcroot(obj, 0); 
  JavaLock* res = 0;
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
  
    JavaLock** tab = LockTable[index >> BitIndex];
  
    if (tab == NULL) {
      tab = (JavaLock**)associatedVM->allocator.Allocate(
          IndexSize * sizeof(JavaLock*), "Index LockTable");
    }
    threadLock.unlock();
   
    // Allocate the lock.
    res = new(associatedVM->allocator, "Lock") JavaLock(index, obj);
    
    // Add the lock to the table.
    uint32_t internalIndex = index & BitMask;
    tab[internalIndex] = res;
  }
   
  // Return the lock.
  return res;
}

void LockSystem::deallocate(JavaLock* lock) {
  lock->associatedObject = NULL;
  threadLock.lock();
  lock->nextFreeLock = freeLock;
  freeLock = lock;
  threadLock.unlock();
}
  
LockSystem::LockSystem(Jnjvm* vm) {
  associatedVM = vm;
  LockTable = (JavaLock* **)
    vm->allocator.Allocate(GlobalSize * sizeof(JavaLock**), "Global LockTable");
  LockTable[0] = (JavaLock**)
    vm->allocator.Allocate(IndexSize * sizeof(JavaLock*), "Index LockTable");
  currentIndex = 0;
  freeLock = NULL;
}

uintptr_t JavaLock::getID() {
  return (index << mvm::NonLockBits) | mvm::FatMask;
}

JavaLock* JavaLock::getFromID(uintptr_t ID) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  if (ID & mvm::FatMask) {
    uint32_t index = (ID & ~mvm::FatMask) >> mvm::NonLockBits;
    JavaLock* res = vm->lockSystem.getLock(index);
    return res;
  } else {
    return 0;
  }
}

void JavaLock::release(JavaObject* obj) {
  llvm_gcroot(obj, 0);
  assert(associatedObject && "No associated object when releasing");
  assert(associatedObject == obj && "Mismatch object in lock");
  if (!waitingThreads && !lockingThreads &&
      internalLock.recursionCount() == 1) {
    associatedObject->lock.initialise();
    deallocate();
  }
  internalLock.unlock();
}

/// acquire - Acquires the internalLock.
///
bool JavaLock::acquire(JavaObject* obj) {
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
