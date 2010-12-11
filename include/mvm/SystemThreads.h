#ifndef _SYSTEM_THREADS_H_
#define _SYSTEM_THREADS_H_

#include "MutatorThread.h"
#include "mvm/Threads/Cond.h"

// Same values than JikesRVM
#define INITIAL_QUEUE_SIZE 256
#define GROW_FACTOR 2

namespace mvm {
class VirtualMachine;

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
 
  void addReference(mvm::gc* ref);
  
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
  void addWeakReference(mvm::gc* ref);
  
  /// addSoftReference - Add a weak reference to the queue.
  ///
  void addSoftReference(mvm::gc* ref);
  
  /// addPhantomReference - Add a weak reference to the queue.
  ///
  void addPhantomReference(mvm::gc* ref);

  ReferenceThread(mvm::VMKit* vmkit);

	virtual ~ReferenceThread() {
    delete[] ToEnqueue;
  }

  virtual void tracer(uintptr_t closure);
};

class FinalizerThread : public mvm::MutatorThread {
public:
    /// FinalizationQueueLock - A lock to protect access to the queue.
  ///
  mvm::SpinLock FinalizationQueueLock;

  /// finalizationQueue - A list of allocated objets that contain a finalize
  /// method.
  ///
	mvm::gc** FinalizationQueue;

  /// CurrentIndex - Current index in the queue of finalizable objects.
  ///
  uint32 CurrentIndex;

  /// QueueLength - Current length of the queue of finalizable objects.
  ///
  uint32 QueueLength;

  /// growFinalizationQueue - Grow the queue of finalizable objects.
  ///
  void growFinalizationQueue();
  
  /// ToBeFinalized - List of objects that are scheduled to be finalized.
  ///
	mvm::gc** ToBeFinalized;
  
  /// ToBeFinalizedLength - Current length of the queue of objects scheduled
  /// for finalization.
  ///
  uint32 ToBeFinalizedLength;

  /// CurrentFinalizedIndex - The current index in the ToBeFinalized queue
  /// that will be sceduled for finalization.
  ///
  uint32 CurrentFinalizedIndex;
  
  /// growToBeFinalizedQueue - Grow the queue of the to-be finalized objects.
  ///
  void growToBeFinalizedQueue();
  
  /// finalizationCond - Condition variable to wake up finalization threads.
  ///
  mvm::Cond FinalizationCond;

  /// finalizationLock - Lock for the condition variable.
  ///
  mvm::LockNormal FinalizationLock;

  static void finalizerStart(FinalizerThread*);

  /// addFinalizationCandidate - Add an object to the queue of objects with
  /// a finalization method.
  ///
  void addFinalizationCandidate(mvm::gc*);

  /// scanFinalizationQueue - Scan objets with a finalized method and schedule
  /// them for finalization if they are not live.
  ///
  void scanFinalizationQueue(uintptr_t closure);

  FinalizerThread(VMKit* vmkit);

	~FinalizerThread() {
    delete[] FinalizationQueue;
    delete[] ToBeFinalized;
  }

  virtual void tracer(uintptr_t closure);
};

} // namespace mvm


#endif
