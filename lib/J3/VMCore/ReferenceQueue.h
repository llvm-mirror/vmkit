//===---ReferenceQueue.h - Implementation of soft/weak/phantom references--===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_REFERENCE_QUEUE_H
#define J3_REFERENCE_QUEUE_H

#include "mvm/Threads/Locks.h"

#include "JavaThread.h"

// Same values than JikesRVM
#define INITIAL_QUEUE_SIZE 256
#define GROW_FACTOR 2

namespace j3 {

class ReferenceThread;
class Jnjvm;

class ReferenceQueue {
private:
	mvm::gc** References;
  uint32 QueueLength;
  uint32 CurrentIndex;
  mvm::SpinLock QueueLock;
  uint8_t semantics;

	mvm::gc* processReference(mvm::gc*, ReferenceThread*, uintptr_t closure);
public:

  static const uint8_t WEAK = 1;
  static const uint8_t SOFT = 2;
  static const uint8_t PHANTOM = 3;


  ReferenceQueue(uint8_t s) {
    References = new mvm::gc*[INITIAL_QUEUE_SIZE];
    memset(References, 0, INITIAL_QUEUE_SIZE * sizeof(mvm::gc*));
    QueueLength = INITIAL_QUEUE_SIZE;
    CurrentIndex = 0;
    semantics = s;
  }

  ~ReferenceQueue() {
    delete[] References;
  }
 
  void addReference(mvm::gc* ref) {
    llvm_gcroot(ref, 0);
    QueueLock.acquire();
    if (CurrentIndex >= QueueLength) {
      uint32 newLength = QueueLength * GROW_FACTOR;
			mvm::gc** newQueue = new mvm::gc*[newLength];
      memset(newQueue, 0, newLength * sizeof(mvm::gc*));
      if (!newQueue) {
        fprintf(stderr, "I don't know how to handle reference overflow yet!\n");
        abort();
      }
      for (uint32 i = 0; i < QueueLength; ++i) newQueue[i] = References[i];
      delete[] References;
      References = newQueue;
      QueueLength = newLength;
    }
    References[CurrentIndex++] = ref;
    QueueLock.release();
  }
  
  void acquire() {
    QueueLock.acquire();
  }

  void release() {
    QueueLock.release();
  }

  void scan(ReferenceThread* thread, uintptr_t closure);
};

class ReferenceThread : public mvm::MutatorThread {
public:
  /// WeakReferencesQueue - The queue of weak references.
  ///
  ReferenceQueue WeakReferencesQueue;

  /// SoftReferencesQueue - The queue of soft references.
  ///
  ReferenceQueue SoftReferencesQueue;

  /// PhantomReferencesQueue - The queue of phantom references.
  ///
  ReferenceQueue PhantomReferencesQueue;

	mvm::gc** ToEnqueue;
  uint32 ToEnqueueLength;
  uint32 ToEnqueueIndex;
  
  /// ToEnqueueLock - A lock to protect access to the queue.
  ///
  mvm::LockNormal EnqueueLock;
  mvm::Cond EnqueueCond;
  mvm::SpinLock ToEnqueueLock;

  void addToEnqueue(mvm::gc* obj);

  static void enqueueStart(ReferenceThread*);

  /// addWeakReference - Add a weak reference to the queue.
  ///
  void addWeakReference(mvm::gc* ref) {
    llvm_gcroot(ref, 0);
    WeakReferencesQueue.addReference(ref);
  }
  
  /// addSoftReference - Add a weak reference to the queue.
  ///
  void addSoftReference(mvm::gc* ref) {
    llvm_gcroot(ref, 0);
    SoftReferencesQueue.addReference(ref);
  }
  
  /// addPhantomReference - Add a weak reference to the queue.
  ///
  void addPhantomReference(mvm::gc* ref) {
    llvm_gcroot(ref, 0);
    PhantomReferencesQueue.addReference(ref);
  }

  ReferenceThread(mvm::VMKit* vmkit);

	virtual void localDestroy() {
    delete[] ToEnqueue;
  }
};

} // namespace j3

#endif  //J3_REFERENCE_QUEUE_H
