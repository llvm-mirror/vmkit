//===--------------- JavaLocks.h - Fat lock management --------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JAVA_LOCKS_H
#define JAVA_LOCKS_H

#include "mvm/Allocator.h"
#include "mvm/Threads/Locks.h"

namespace mvm {
  class Thread;
}

namespace j3 {

class JavaObject;
class JavaThread;
class Jnjvm;

class JavaLock : public mvm::PermanentObject {

friend class JavaObject;
friend class LockSystem;

private:
  mvm::LockRecursive internalLock;
  mvm::SpinLock spinLock;
  uint32_t waitingThreads;
  uint32_t lockingThreads;
  JavaThread* firstThread;
  JavaObject* associatedObject;
  uint32_t index;
  JavaLock* nextFreeLock;

public:

  JavaObject* getAssociatedObject() {
    return associatedObject;
  }

  /// acquire - Acquires the internalLock.
  ///
  bool acquire(JavaObject* obj) {
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
 
  /// tryAcquire - Tries to acquire the lock.
  ///
  int tryAcquire() {
    return internalLock.tryLock();
  }

 
  /// acquireAll - Acquires the lock nb times.
  void acquireAll(uint32 nb) {
    internalLock.lockAll(nb);
  }

  /// release - Releases the internalLock.
  ///
  void release(JavaObject* obj);
  
  /// owner - Returns if the current thread owns this internalLock.
  ///
  bool owner() {
    return internalLock.selfOwner();
  }
 
  /// getOwner - Get the owner of this internalLock.
  ///
  mvm::Thread* getOwner() {
    return internalLock.getOwner();
  }
  
  /// JavaLock - Default constructor.
  JavaLock(uint32_t i, JavaObject* a) {
    llvm_gcroot(a, 0);
    firstThread = 0;
    index = i;
    associatedObject = a;
    waitingThreads = 0;
    lockingThreads = 0;
  }

  static JavaLock* allocate(JavaObject*);
  void deallocate();
  
  uintptr_t getID();
  static JavaLock* getFromID(uintptr_t val);
};

/// LockSystem - This class manages all Java locks used by the applications.
/// Each JVM must own an instance of this class and allocate Java locks
/// with it.
///
class LockSystem {
  friend class JavaLock;
public:
  
  // Fixed values. With these values, an index is on 18 bits.
  static const uint32_t GlobalSize = 128;
  static const uint32_t BitIndex = 11;
  static const uint32_t IndexSize = 1 << BitIndex;
  static const uint32_t BitMask = IndexSize - 1;
  static const uint32_t MaxLocks = GlobalSize * IndexSize;
  static const uint32_t BitGC = 12;

  /// LockTable - The global table that will hold the locks. The table is
  /// a two-dimensional array, and only one entry is created, so that
  /// the lock system does not eat up all memory on startup.
  ///  
  JavaLock* ** LockTable;
  
  /// currentIndex - The current index in the tables. Always incremented,
  /// never decremented.
  ///
  uint32_t currentIndex;
 
  /// freeLock - The list of locks that are allocated and available.
  ///
  JavaLock* freeLock;
 
  /// threadLock - Spin lock to protect the currentIndex field.
  ///
  mvm::SpinLock threadLock;
  
  /// associatedVM - The JVM associated with this lock system.
  ///
  Jnjvm* associatedVM;  

  /// allocate - Allocate a JavaLock.
  ///
  JavaLock* allocate(JavaObject* obj); 
 
  /// deallocate - Put a lock in the free list lock.
  ///
  void deallocate(JavaLock* lock) {
    lock->associatedObject = 0;
    threadLock.lock();
    lock->nextFreeLock = freeLock;
    freeLock = lock;
    threadLock.unlock();
  }

  /// LockSystem - Default constructor. Initialize the table.
  ///
  LockSystem(Jnjvm* vm);

  /// getLock - Get a lock from an index in the table.
  ///
  JavaLock* getLock(uint32_t index) {
    return LockTable[index >> BitIndex][index & BitMask];
  }
};

}

#endif
