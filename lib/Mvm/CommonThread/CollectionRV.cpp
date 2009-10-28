//===-------- CollectionRV.cpp - Rendez-vous for garbage collection -------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include "MvmGC.h"

using namespace mvm;

void CollectionRV::waitCollection() {
  mvm::Thread* th = mvm::Thread::get();
  unsigned cm = rendezvousNb;

  if (nbCollected == th->MyVM->NumberOfThreads) collectorGo();
  
  while (rendezvousNb == cm) {
    collectionCond.wait(&_stackLock);
  }
}

void CollectionRV::waitStacks() {
  mvm::Thread* self = mvm::Thread::get();
  stackLock();
  
  while (nbCollected < self->MyVM->NumberOfThreads)
    stackCond.wait(&_stackLock);
  
  stackUnlock();
}

void CollectionRV::synchronize() {
  
  mvm::Thread* self = mvm::Thread::get();
  assert(self && "No thread local data for this thread");

  // Lock thread lock, so that we can traverse the thread list safely.
  self->MyVM->ThreadLock.lock();

  if (cooperative) {
    currentCollector = self;
    nbCollected = 0;
 	 
    // Lock stacks. Changes on the doYield flag of threads must be
    // protected, as threads may wake up after being blocked in native code
    // and join the collection.
    stackLock();

    mvm::Thread* cur = self;
    do {
      cur->stackScanned = false;
      cur->doYield = true;
      cur = (mvm::Thread*)cur->next();
    } while (cur != self);
    
    // Unlock now. Each running thread will scan its stack.
    stackUnlock();

    // Scan the stacks of currently blocked threads.
    for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      void* val = cur->getLastSP();
      // If val is null, this means that the thread woke up, and is
      // joining the collection. We are sure the thread will scan its stack.
      if (val) addForeignThread(cur);
    }

    // Add myself.
    another_mark();

    // And wait for other threads to finish.
    waitStacks();

  } else {
    mvm::Thread* self = mvm::Thread::get();
    self->stackScanned = false;
    assert(self && "No thread local data for this thread");
    currentCollector = self;
    nbCollected = 0;

    for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      cur->stackScanned = false;
      cur->killForRendezvous();
    }
    
    // Add myself.
    another_mark();

    // And wait for other threads to finish.
    waitStacks();
  }
 
  self->MyVM->ThreadLock.unlock();
}

void CollectionRV::join() {
  mvm::Thread* th = mvm::Thread::get();
  th->inGC = true;
  bool changed = false;
 
  stackLock();

  if (isCooperative() && !th->doYield) {
    // I was previously blocked, and I'm running late: someone else collected
    // my stack, and the GC has finished already. Just unlock and return.
    stackUnlock();
    th->inGC = false;
    return;
  }
 
  // I woke up while a GC was happening, and no-one has listed me yet.
  if (!th->stackScanned) {
    another_mark();
    th->stackScanned = true;
    if (!th->getLastSP()) {
      changed = true;
      th->setLastSP(FRAME_PTR());
    }
  }

  // Wait for the collection to finish.
  waitCollection();
  stackUnlock();
  
  // The collection is finished. Set inGC to false.
  th->inGC = false;
  if (changed) th->setLastSP(0);
}

void CollectionRV::addForeignThread(mvm::Thread* th) {
  stackLock();

  // The thread may have waken up during this GC. In this case, it may have
  // put itself into the waiting list.
  if (!th->stackScanned) {
    another_mark();
    th->stackScanned = true;
  }

  stackUnlock();
}
