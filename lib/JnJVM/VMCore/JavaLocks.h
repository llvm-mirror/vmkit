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

namespace jnjvm {

class JavaObject;
class JavaThread;
class Jnjvm;

class JavaLock : public mvm::PermanentObject {

friend class JavaObject;
friend class LockSystem;

private:
  mvm::LockRecursive internalLock;
  mvm::Thread* waitingThread;
  JavaThread* firstThread;
  JavaObject* associatedObject;
  uint32_t index;
  JavaLock* nextFreeLock;

public:

  /// acquire - Acquires the internalLock.
  ///
  void acquire() {
    waitingThread = mvm::Thread::get();
    internalLock.lock();
    waitingThread = 0;
  }
 
  /// tryAcquire - Tries to acquire the lock.
  ///
  int tryAcquire() {
    return internalLock.tryLock();
  }

 
  /// acquireAll - Acquires the lock nb times.
  void acquireAll(uint32 nb) {
    waitingThread = mvm::Thread::get();
    internalLock.lockAll(nb);
    waitingThread = 0;
  }

  /// release - Releases the internalLock.
  ///
  void release() {
    internalLock.unlock();
  }
  
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
    firstThread = 0;
    index = i;
    associatedObject = a;
    waitingThread = 0;
  }

  static JavaLock* allocate(JavaObject*);
  void deallocate();
  
};

/// LockSystem - This class manages all Java locks used by the applications.
/// Each JVM must own an instance of this class and allocate Java locks
/// with it.
///
class LockSystem {
public:
  
  // Fixed values. With these values, an index is on 18 bits.
  static const uint32_t GlobalSize = 128;
  static const uint32_t BitIndex = 11;
  static const uint32_t IndexSize = 1 << BitIndex;
  static const uint32_t BitMask = IndexSize - 1;
  static const uint32_t MaxLocks = GlobalSize * IndexSize;

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
