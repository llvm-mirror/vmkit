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

class JavaLock : public mvm::PermanentObject {

friend class JavaObject;

private:
  mvm::LockRecursive internalLock;
  mvm::Thread* waitingThread;
  JavaThread* firstThread;
  JavaObject* associatedObject;
  uint32_t index;

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
  
  /// JavaLock - Empty constructor.
  JavaLock(uint32_t i) {
    firstThread = 0;
    index = i;
    associatedObject = 0;
    waitingThread = 0;
  }

  static JavaLock* allocate(JavaObject*);
  
};

}

#endif
