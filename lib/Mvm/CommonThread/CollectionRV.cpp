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

void CollectionRV::waitEndOfRV() {
  mvm::Thread* th = mvm::Thread::get();
  unsigned cm = rendezvousNb;

  if (nbJoined == th->MyVM->NumberOfThreads) collectorGo();
  
  while (rendezvousNb == cm) {
    condEndRV.wait(&_lockRV);
  }
}

void CollectionRV::waitRV() {
  mvm::Thread* self = mvm::Thread::get();
  lockRV();
  
  while (nbJoined < self->MyVM->NumberOfThreads)
    condInitiator.wait(&_lockRV);
  
  unlockRV();
}

void CollectionRV::synchronize() {
  
  mvm::Thread* self = mvm::Thread::get();
  assert(self && "No thread local data for this thread");
  self->inRV = true;

  // Lock thread lock, so that we can traverse the thread list safely.
  self->MyVM->ThreadLock.lock();

  if (cooperative) {
    initiator = self;
    nbJoined = 0;
 	 
    // Lock. Changes on the doYield flag of threads must be protected, as
    // threads may wake up after being blocked in native code and join the
    // rendezvous.
    lockRV();

    mvm::Thread* cur = self;
    do {
      cur->joinedRV = false;
      cur->doYield = true;
      cur = (mvm::Thread*)cur->next();
    } while (cur != self);
    
    // Unlock now. Each running thread will join the rendezvous.
    unlockRV();

    // Lookup currently blocked threads.
    for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      void* val = cur->getLastSP();
      // If val is null, this means that the thread woke up, and is
      // joining the rendezvous. We are sure the thread will join the
      // rendezvous.
      if (val) addForeignThread(cur);
    }

    // Add myself.
    another_mark();

    // And wait for other threads to join.
    waitRV();

  } else {
    mvm::Thread* self = mvm::Thread::get();
    self->joinedRV = false;
    assert(self && "No thread local data for this thread");
    initiator = self;
    nbJoined = 0;

    for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      cur->joinedRV = false;
      cur->killForRendezvous();
    }
    
    // Add myself.
    another_mark();

    // And wait for other threads to finish.
    waitRV();
  }
 
  self->MyVM->ThreadLock.unlock();
}

void CollectionRV::join() {
  mvm::Thread* th = mvm::Thread::get();
  th->inRV = true;
  bool changed = false;
 
  lockRV();

  if (isCooperative() && !th->doYield) {
    // I was previously blocked, and I'm running late: someone else collected
    // my stack, and the GC has finished already. Just unlock and return.
    unlockRV();
    th->inRV = false;
    return;
  }
 
  // I woke up while a GC was happening, and no-one has listed me yet.
  if (!th->joinedRV) {
    another_mark();
    th->joinedRV = true;
    if (!th->getLastSP()) {
      changed = true;
      th->setLastSP(FRAME_PTR());
    }
  }

  // Wait for the rendezvous to finish.
  waitEndOfRV();
  unlockRV();
  
  // The rendezvous is finished. Set inRV to false.
  th->inRV = false;
  if (changed) th->setLastSP(0);
}

void CollectionRV::addForeignThread(mvm::Thread* th) {
  lockRV();

  // The thread may have waken up during this GC. In this case, it may have
  // put itself into the waiting list.
  if (!th->joinedRV) {
    another_mark();
    th->joinedRV = true;
  }

  unlockRV();
}

extern "C" void conditionalSafePoint() {
  mvm::Thread* th = mvm::Thread::get();
  th->startNative(1);
  th->MyVM->rendezvous.join();
  th->endNative();
}
