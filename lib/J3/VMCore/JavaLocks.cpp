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
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaLock* res = vm->lockSystem.allocate(obj);  
  return res;
}

void JavaLock::deallocate() {
  Jnjvm* vm = JavaThread::get()->getJVM();
  vm->lockSystem.deallocate(this);
}

JavaLock* LockSystem::allocate(JavaObject* obj) { 
  
  JavaLock* res = 0;
  threadLock.lock();

  // Try the freeLock list.
  if (freeLock) {
    res = freeLock;
    freeLock = res->nextFreeLock;
    res->nextFreeLock = 0;
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
  
    if (tab == NULL) 
      tab = (JavaLock**)associatedVM->allocator.Allocate(IndexSize,
                                                         "Index LockTable");
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
  
LockSystem::LockSystem(Jnjvm* vm) {
  associatedVM = vm;
  LockTable = (JavaLock* **)
    vm->allocator.Allocate(GlobalSize, "Global LockTable");
  LockTable[0] = (JavaLock**)
    vm->allocator.Allocate(IndexSize, "Index LockTable");
  currentIndex = 0;
}

uintptr_t JavaLock::getID() {
  return (index << LockSystem::BitGCHash) | mvm::FatMask;
}

JavaLock* JavaLock::getFromID(uintptr_t ID) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  if (ID & mvm::FatMask) {
    uint32_t index = (ID & ~mvm::FatMask) >> LockSystem::BitGCHash;
    JavaLock* res = vm->lockSystem.getLock(index);
    return res;
  } else {
    return 0;
  }
}

void JavaLock::release(JavaObject* obj) {
  assert(associatedObject == obj && "Mismatch object in lock");
  llvm_gcroot(obj, 0);
  if (!waitingThreads && !lockingThreads &&
      internalLock.recursionCount() == 1) {
    assert(associatedObject && "No associated object when releasing");
    associatedObject->lock.initialise();
    deallocate();
  }
  internalLock.unlock();
}
