//===--ReferenceQueue.cpp - Implementation of soft/weak/phantom references-===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "ReferenceQueue.h"

using namespace j3;

bool enqueueReference(gc* _obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaObject* obj = (JavaObject*)_obj;
  llvm_gcroot(obj, 0);
  JavaMethod* meth = vm->upcalls->EnqueueReference;
  UserClass* cl = JavaObject::getClass(obj)->asClass();
  return (bool)meth->invokeIntSpecialBuf(vm, cl, obj, 0);
}

void Jnjvm::invokeEnqueueReference(gc* res) {
  llvm_gcroot(res, 0);
  TRY {
    enqueueReference(res);
  } IGNORE;
  vmkit::Thread::get()->clearException();
}

gc** Jnjvm::getObjectReferentPtr(gc* _obj) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  llvm_gcroot(obj, 0);
  return (gc**)JavaObjectReference::getReferentPtr(obj);
}

void Jnjvm::setObjectReferent(gc* _obj, gc* val) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  llvm_gcroot(obj, 0);
  llvm_gcroot(val, 0);
  JavaObjectReference::setReferent(obj, (JavaObject*)val);
}
 
void Jnjvm::clearObjectReferent(gc* _obj) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  llvm_gcroot(obj, 0);
  JavaObjectReference::setReferent(obj, NULL);
}

FinalizerThread::FinalizerThread(Jnjvm* vm) : JavaThread(vm) {
  FinalizationQueue = new gc*[INITIAL_QUEUE_SIZE];
  QueueLength = INITIAL_QUEUE_SIZE;
  CurrentIndex = 0;

  ToBeFinalized = new gc*[INITIAL_QUEUE_SIZE];
  ToBeFinalizedLength = INITIAL_QUEUE_SIZE;
  CurrentFinalizedIndex = 0;
}

void FinalizerThread::growFinalizationQueue() {
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

void FinalizerThread::growToBeFinalizedQueue() {
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


void FinalizerThread::addFinalizationCandidate(gc* obj) {
  llvm_gcroot(obj, 0);
  FinalizationQueueLock.acquire();
 
  if (CurrentIndex >= QueueLength) {
    growFinalizationQueue();
  }
  
  FinalizationQueue[CurrentIndex++] = obj;
  FinalizationQueueLock.release();
}
  

void FinalizerThread::scanFinalizationQueue(word_t closure) {
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


void FinalizerThread::finalizerStart(FinalizerThread* th) {
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
