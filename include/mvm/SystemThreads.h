#ifndef _SYSTEM_THREADS_H_
#define _SYSTEM_THREADS_H_

#include "MutatorThread.h"
#include "mvm/Threads/Cond.h"

// Same values than JikesRVM
#define INITIAL_QUEUE_SIZE 256
#define GROW_FACTOR 2

namespace mvm {
class VirtualMachine;

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
