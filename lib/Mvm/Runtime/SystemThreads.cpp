#include "mvm/SystemThreads.h"
#include "mvm/GC.h"
#include "mvm/VirtualMachine.h"

using namespace mvm;

FinalizerThread::FinalizerThread(VMKit* vmkit) : MutatorThread(vmkit) {
  FinalizationQueue = new mvm::gc*[INITIAL_QUEUE_SIZE];
  QueueLength = INITIAL_QUEUE_SIZE;
  CurrentIndex = 0;

  ToBeFinalized = new mvm::gc*[INITIAL_QUEUE_SIZE];
  ToBeFinalizedLength = INITIAL_QUEUE_SIZE;
  CurrentFinalizedIndex = 0;
}

void FinalizerThread::growFinalizationQueue() {
  if (CurrentIndex >= QueueLength) {
    uint32 newLength = QueueLength * GROW_FACTOR;
		mvm::gc** newQueue = new mvm::gc*[newLength];
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

void FinalizerThread::growToBeFinalizedQueue() {
  if (CurrentFinalizedIndex >= ToBeFinalizedLength) {
    uint32 newLength = ToBeFinalizedLength * GROW_FACTOR;
		mvm::gc** newQueue = new mvm::gc*[newLength];
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


void FinalizerThread::addFinalizationCandidate(mvm::gc* obj) {
  llvm_gcroot(obj, 0);
  FinalizationQueueLock.acquire();
 
  if (CurrentIndex >= QueueLength) {
    growFinalizationQueue();
  }
  
  FinalizationQueue[CurrentIndex++] = obj;
  FinalizationQueueLock.release();
}

void FinalizerThread::scanFinalizationQueue(uintptr_t closure) {
  uint32 NewIndex = 0;
  for (uint32 i = 0; i < CurrentIndex; ++i) {
		mvm::gc* obj = FinalizationQueue[i];

    if (!mvm::Collector::isLive(obj, closure)) {
      obj = mvm::Collector::retainForFinalize(FinalizationQueue[i], closure);
      
      if (CurrentFinalizedIndex >= ToBeFinalizedLength)
        growToBeFinalizedQueue();
      
      /* Add to object table */
      ToBeFinalized[CurrentFinalizedIndex++] = obj;
    } else {
      FinalizationQueue[NewIndex++] =
        mvm::Collector::getForwardedFinalizable(obj, closure);
    }
  }
  CurrentIndex = NewIndex;
}

typedef void (*destructor_t)(void*);

void invokeFinalize(mvm::gc* obj) {
  llvm_gcroot(obj, 0);
  TRY {
		llvm_gcroot(obj, 0);
		VirtualMachine* vm = obj->getVirtualTable()->vm; //JavaThread::get()->getJVM();
		mvm::Thread::get()->attach(vm);
		vm->finalizeObject(obj);
  } IGNORE;
  mvm::Thread::get()->clearPendingException();
}

void FinalizerThread::finalizerStart(FinalizerThread* th) {
	mvm::gc* res = NULL;
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

			mvm::VirtualTable* VT = res->getVirtualTable();
			ASSERT(VT->vm);
      if (VT->operatorDelete) {
        destructor_t dest = (destructor_t)VT->destructor;
        dest(res);
      } else {
        invokeFinalize(res);
      }
      res = NULL;
    }
  }
}

void FinalizerThread::tracer(uintptr_t closure) {
  // (5) Trace the finalization queue.
  for (uint32 i = 0; i < CurrentFinalizedIndex; ++i) {
    mvm::Collector::markAndTraceRoot(ToBeFinalized + i, closure);
  }
	MutatorThread::tracer(closure);
}
