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

ReferenceThread::ReferenceThread(mvm::VMKit* vmkit) :
	MutatorThread(vmkit),
	WeakReferencesQueue(ReferenceQueue::WEAK),
	SoftReferencesQueue(ReferenceQueue::SOFT), 
	PhantomReferencesQueue(ReferenceQueue::PHANTOM) {

  ToEnqueue = new mvm::gc*[INITIAL_QUEUE_SIZE];
  ToEnqueueLength = INITIAL_QUEUE_SIZE;
  ToEnqueueIndex = 0;
}

mvm::gc** getReferent(mvm::gc* obj) {
  llvm_gcroot(obj, 0);
	mvm::VirtualMachine* vm = obj->getVirtualTable()->vm;
	mvm::Thread::get()->attach(vm);
	return vm->getReferent(obj);
}

void setReferent(mvm::gc* obj, mvm::gc* val) {
	printf("set referent: %p %p\n", obj, val);
  llvm_gcroot(obj, 0);
  llvm_gcroot(val, 0);
	mvm::VirtualMachine* vm = obj->getVirtualTable()->vm;
	mvm::Thread::get()->attach(vm);
	vm->setReferent(obj, val);
}
 
void invokeEnqueue(mvm::gc* obj) {
  llvm_gcroot(obj, 0);
  TRY {
		mvm::VirtualMachine* vm = obj->getVirtualTable()->vm;
		mvm::Thread::get()->attach(vm);
		
    vm->enqueueReference(obj);
  } IGNORE;
  mvm::Thread::get()->clearPendingException();
}

void ReferenceThread::enqueueStart(ReferenceThread* th) {
	mvm::gc* res = NULL;
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

      invokeEnqueue(res);
      res = NULL;
    }
  }
}


void ReferenceThread::addToEnqueue(mvm::gc* obj) {
  llvm_gcroot(obj, 0);
  if (ToEnqueueIndex >= ToEnqueueLength) {
    uint32 newLength = ToEnqueueLength * GROW_FACTOR;
		mvm::gc** newQueue = new mvm::gc*[newLength];
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

mvm::gc* ReferenceQueue::processReference(mvm::gc* reference, ReferenceThread* th, uintptr_t closure) {
  if (!mvm::Collector::isLive(reference, closure)) {
    setReferent(reference, 0);
    return NULL;
  }

	mvm::gc* referent = *(getReferent(reference));

  if (!referent) {
    return NULL;
  }

  if (semantics == SOFT) {
    // TODO: are we are out of memory? Consider that we always are for now.
    if (false) {
      mvm::Collector::retainReferent(referent, closure);
    }
  } else if (semantics == PHANTOM) {
    // Nothing to do.
  }

	mvm::gc* newReference =
      mvm::Collector::getForwardedReference(reference, closure);
  if (mvm::Collector::isLive(referent, closure)) {
		mvm::gc* newReferent = mvm::Collector::getForwardedReferent(referent, closure);
    setReferent(newReference, newReferent);
    return newReference;
  } else {
    setReferent(newReference, 0);
    th->addToEnqueue(newReference);
    return NULL;
  }
}


void ReferenceQueue::scan(ReferenceThread* th, uintptr_t closure) {
  uint32 NewIndex = 0;

  for (uint32 i = 0; i < CurrentIndex; ++i) {
		mvm::gc* obj = References[i];
		mvm::gc* res = processReference(obj, th, closure);
    if (res) References[NewIndex++] = res;
  }

  CurrentIndex = NewIndex;
}

