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
  // Add myself.
  another_mark();

  while (nbJoined < self->MyVM->NumberOfThreads)
    condInitiator.wait(&_lockRV);
  
}

void CollectionRV::synchronize() {
  
  mvm::Thread* self = mvm::Thread::get();
  assert(self && "No thread local data for this thread");
  self->inRV = true;
  
  // Lock thread lock, so that we can traverse the thread list safely. This will
  // be released on finishRV.
  self->MyVM->ThreadLock.lock();

  if (cooperative) {
 	 
    mvm::Thread* cur = self;
    do {
      cur->joinedRV = false;
      cur->doYield = true;
      cur = (mvm::Thread*)cur->next();
    } while (cur != self);
   
    // Lookup currently blocked threads.
    for (cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      if (cur->getLastSP()) {
        another_mark();
        cur->joinedRV = true;
      }
    }
    
  } else {
    mvm::Thread* self = mvm::Thread::get();
    self->joinedRV = false;
    assert(self && "No thread local data for this thread");

    for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      cur->joinedRV = false;
      cur->killForRendezvous();
    }
    
  }
  
  // And wait for other threads to finish.
  waitRV();
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
  }
    
  // lastSP may not be set in two cases:
  // (1) The thread was interrupted while executing regular code (ie cooperative
  //     code).
  // (2) The thread left uncooperative code and has just cleared lastSP.
  if (!th->getLastSP()) {
    changed = true;
    th->setLastSP(FRAME_PTR());
  }

  assert(th->getLastSP() && "Joined without giving a SP");
  // Wait for the rendezvous to finish.
  waitEndOfRV();
  
  // The rendezvous is finished. Set inRV to false.
  th->inRV = false;
  if (changed) th->setLastSP(0);
 
  // Unlock after modifying lastSP, because lastSP is also read by the
  // rendezvous initiator.
  unlockRV();
  
}

extern "C" void conditionalSafePoint() {
  mvm::Thread* th = mvm::Thread::get();
  th->MyVM->rendezvous.join();
}

void CollectionRV::finishRV() {
    
  mvm::Thread* self = mvm::Thread::get();
  if (cooperative) {
    mvm::Thread* initiator = mvm::Thread::get();
    mvm::Thread* cur = initiator;
    do {
      cur->doYield = false;
      cur = (mvm::Thread*)cur->next();
    } while (cur != initiator);
  }

  nbJoined = 0;
  rendezvousNb++;
  condEndRV.broadcast();
  self->inRV = false;
  self->MyVM->ThreadLock.unlock();
  
  unlockRV();
}
