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

#include "vmkit/Locks.h"

#include "JavaThread.h"

// Same values than JikesRVM
#define INITIAL_QUEUE_SIZE 256
#define GROW_FACTOR 2

namespace j3 {

template <class T> class ReferenceThread;
class Jnjvm;


class ReferenceQueue {
private:
  gc** References;
  uint32 QueueLength;
  uint32 CurrentIndex;
  vmkit::SpinLock QueueLock;
  uint8_t semantics;

  template <class T>
  gc* processReference(gc* reference, ReferenceThread<T>* th, word_t closure) {
    if (!vmkit::Collector::isLive(reference, closure)) {
      vmkit::Thread::get()->MyVM->clearObjectReferent(reference);
      return NULL;
    }

    gc* referent = *(vmkit::Thread::get()->MyVM->getObjectReferentPtr(reference));

    if (!referent) {
      return NULL;
    }

    if (semantics == SOFT) {
      // TODO: are we are out of memory? Consider that we always are for now.
      if (false) {
        vmkit::Collector::retainReferent(referent, closure);
      }
    } else if (semantics == PHANTOM) {
      // Nothing to do.
    }

    gc* newReference =
        vmkit::Collector::getForwardedReference(reference, closure);
    if (vmkit::Collector::isLive(referent, closure)) {
      gc* newReferent = vmkit::Collector::getForwardedReferent(referent, closure);
      vmkit::Thread::get()->MyVM->setObjectReferent(newReference, newReferent);
      return newReference;
    } else {
    	vmkit::Thread::get()->MyVM->clearObjectReferent(newReference);
      th->addToEnqueue(newReference);
      return NULL;
    }
  }

public:

  static const uint8_t WEAK = 1;
  static const uint8_t SOFT = 2;
  static const uint8_t PHANTOM = 3;


  ReferenceQueue(uint8_t s) {
    References = new gc*[INITIAL_QUEUE_SIZE];
    memset(References, 0, INITIAL_QUEUE_SIZE * sizeof(gc*));
    QueueLength = INITIAL_QUEUE_SIZE;
    CurrentIndex = 0;
    semantics = s;
  }

  ~ReferenceQueue() {
    delete[] References;
  }
 
  void addReference(gc* ref) {
    llvm_gcroot(ref, 0);
    QueueLock.acquire();
    if (CurrentIndex >= QueueLength) {
      uint32 newLength = QueueLength * GROW_FACTOR;
      gc** newQueue = new gc*[newLength];
      if (!newQueue) {
        fprintf(stderr, "I don't know how to handle reference overflow yet!\n");
        abort();
      }
      memset(newQueue, 0, newLength * sizeof(gc*));
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

  template <class T>
  void scan(ReferenceThread<T>* thread, word_t closure) {
    uint32 NewIndex = 0;

    for (uint32 i = 0; i < CurrentIndex; ++i) {
      gc* obj = References[i];
      gc* res = processReference(obj, thread, closure);
      if (res) References[NewIndex++] = res;
    }

    CurrentIndex = NewIndex;
  }

};

template <class T_THREAD> class ReferenceThread : public T_THREAD {
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

  gc** ToEnqueue;
  uint32 ToEnqueueLength;
  uint32 ToEnqueueIndex;

  /// ToEnqueueLock - A lock to protect access to the queue.
  ///
  vmkit::LockNormal EnqueueLock;
  vmkit::Cond EnqueueCond;
  vmkit::SpinLock ToEnqueueLock;

  static void enqueueStart(ReferenceThread* th){
    gc* res = NULL;
    llvm_gcroot(res, 0);

    while (true) {
      th->EnqueueLock.lock();
      while (th->ToEnqueueIndex == 0) {
        th->EnqueueCond.wait(&th->EnqueueLock);
      }
      th->EnqueueLock.unlock();

      while (true) {
        th->ToEnqueueLock.acquire();
        if (th->ToEnqueueIndex != 0) {
          res = th->ToEnqueue[th->ToEnqueueIndex - 1];
          --th->ToEnqueueIndex;
        }
        th->ToEnqueueLock.release();
        if (!res) break;

        vmkit::Thread::get()->MyVM->invokeEnqueueReference(res);
        res = NULL;
      }
    }
  }

  void addToEnqueue(gc* obj) {
    llvm_gcroot(obj, 0);
    if (ToEnqueueIndex >= ToEnqueueLength) {
      uint32 newLength = ToEnqueueLength * GROW_FACTOR;
      gc** newQueue = new gc*[newLength];
      if (!newQueue) {
        fprintf(stderr, "I don't know how to handle reference overflow yet!\n");
        abort();
      }
      for (uint32 i = 0; i < ToEnqueueLength; ++i) {
        newQueue[i] = ToEnqueue[i];
      }
      delete[] ToEnqueue;
      ToEnqueue = newQueue;
      ToEnqueueLength = newLength;
    }
    ToEnqueue[ToEnqueueIndex++] = obj;
  }

  /// addWeakReference - Add a weak reference to the queue.
  ///
  void addWeakReference(gc* ref) {
    llvm_gcroot(ref, 0);
    WeakReferencesQueue.addReference(ref);
  }

  /// addSoftReference - Add a weak reference to the queue.
  ///
  void addSoftReference(gc* ref) {
    llvm_gcroot(ref, 0);
    SoftReferencesQueue.addReference(ref);
  }

  /// addPhantomReference - Add a weak reference to the queue.
  ///
  void addPhantomReference(gc* ref) {
    llvm_gcroot(ref, 0);
    PhantomReferencesQueue.addReference(ref);
  }

  ReferenceThread(vmkit::VirtualMachine* vm) : T_THREAD(vm), WeakReferencesQueue(ReferenceQueue::WEAK),
      									SoftReferencesQueue(ReferenceQueue::SOFT),
      									PhantomReferencesQueue(ReferenceQueue::PHANTOM) {
    ToEnqueue = new gc*[INITIAL_QUEUE_SIZE];
    ToEnqueueLength = INITIAL_QUEUE_SIZE;
    ToEnqueueIndex = 0;
  }

  ~ReferenceThread() {
    delete[] ToEnqueue;
  }
};

template <class T_THREAD> class FinalizerThread : public T_THREAD {
public:
  /// FinalizationQueueLock - A lock to protect access to the queue.
  ///
  vmkit::SpinLock FinalizationQueueLock;

  /// finalizationQueue - A list of allocated objets that contain a finalize
  /// method.
  ///
  gc** FinalizationQueue;

  /// CurrentIndex - Current index in the queue of finalizable objects.
  ///
  uint32 CurrentIndex;

  /// QueueLength - Current length of the queue of finalizable objects.
  ///
  uint32 QueueLength;

  /// growFinalizationQueue - Grow the queue of finalizable objects.
  ///
  void growFinalizationQueue() {
    if (CurrentIndex >= QueueLength) {
      uint32 newLength = QueueLength * GROW_FACTOR;
      gc** newQueue = new gc*[newLength];
      if (!newQueue) {
        fprintf(stderr, "I don't know how to handle finalizer overflows yet!\n");
        abort();
      }
      for (uint32 i = 0; i < QueueLength; ++i) newQueue[i] = FinalizationQueue[i];
      delete[] FinalizationQueue;
      FinalizationQueue = newQueue;
      QueueLength = newLength;
    }
  }

  
  /// ToBeFinalized - List of objects that are scheduled to be finalized.
  ///
  gc** ToBeFinalized;
  
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
  void growToBeFinalizedQueue() {
    if (CurrentFinalizedIndex >= ToBeFinalizedLength) {
      uint32 newLength = ToBeFinalizedLength * GROW_FACTOR;
      gc** newQueue = new gc*[newLength];
      if (!newQueue) {
        fprintf(stderr, "I don't know how to handle finalizer overflows yet!\n");
        abort();
      }
      for (uint32 i = 0; i < ToBeFinalizedLength; ++i) newQueue[i] = ToBeFinalized[i];
      delete[] ToBeFinalized;
      ToBeFinalized = newQueue;
      ToBeFinalizedLength = newLength;
    }
  }

  
  /// finalizationCond - Condition variable to wake up finalization threads.
  ///
  vmkit::Cond FinalizationCond;

  /// finalizationLock - Lock for the condition variable.
  ///
  vmkit::LockNormal FinalizationLock;

  static void finalizerStart(FinalizerThread* th) {
    gc* res = NULL;
    llvm_gcroot(res, 0);

    while (true) {
      th->FinalizationLock.lock();
      while (th->CurrentFinalizedIndex == 0) {
        th->FinalizationCond.wait(&th->FinalizationLock);
      }
      th->FinalizationLock.unlock();

      while (true) {
        th->FinalizationQueueLock.acquire();
        if (th->CurrentFinalizedIndex != 0) {
          res = th->ToBeFinalized[th->CurrentFinalizedIndex - 1];
          --th->CurrentFinalizedIndex;
        }
        th->FinalizationQueueLock.release();
        if (!res) break;

       th->MyVM->finalizeObject(res);

        res = NULL;
      }
    }
  }

  /// addFinalizationCandidate - Add an object to the queue of objects with
  /// a finalization method.
  ///
  void addFinalizationCandidate(gc* obj) {
    llvm_gcroot(obj, 0);
    FinalizationQueueLock.acquire();

    if (CurrentIndex >= QueueLength) {
      growFinalizationQueue();
    }

    FinalizationQueue[CurrentIndex++] = obj;
    FinalizationQueueLock.release();
  }


  /// scanFinalizationQueue - Scan objets with a finalized method and schedule
  /// them for finalization if they are not live.
  ///
  void scanFinalizationQueue(word_t closure) {
    uint32 NewIndex = 0;
    for (uint32 i = 0; i < CurrentIndex; ++i) {
      gc* obj = FinalizationQueue[i];

      if (!vmkit::Collector::isLive(obj, closure)) {
        obj = vmkit::Collector::retainForFinalize(FinalizationQueue[i], closure);

        if (CurrentFinalizedIndex >= ToBeFinalizedLength)
          growToBeFinalizedQueue();

        /* Add to object table */
        ToBeFinalized[CurrentFinalizedIndex++] = obj;
      } else {
        FinalizationQueue[NewIndex++] =
          vmkit::Collector::getForwardedFinalizable(obj, closure);
      }
    }
    CurrentIndex = NewIndex;
  }


  FinalizerThread(vmkit::VirtualMachine* vm) : T_THREAD(vm) {
    FinalizationQueue = new gc*[INITIAL_QUEUE_SIZE];
    QueueLength = INITIAL_QUEUE_SIZE;
    CurrentIndex = 0;

    ToBeFinalized = new gc*[INITIAL_QUEUE_SIZE];
    ToBeFinalizedLength = INITIAL_QUEUE_SIZE;
    CurrentFinalizedIndex = 0;
  }

  ~FinalizerThread() {
    delete[] FinalizationQueue;
    delete[] ToBeFinalized;
  }
};

class JavaFinalizerThread : public FinalizerThread<JavaThread>{
	public:
		JavaFinalizerThread(Jnjvm* vm) : FinalizerThread<JavaThread>(vm) {}
};

class JavaReferenceThread : public ReferenceThread<JavaThread> {
public:
	JavaReferenceThread(Jnjvm* vm) : ReferenceThread<JavaThread>(vm) {}
};


} // namespace j3

#endif  //J3_REFERENCE_QUEUE_H
