//===-------- CollectionRV.cpp - Rendez-vous for garbage collection -------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <signal.h>
#include "VmkitGC.h"
#include "vmkit/VirtualMachine.h"
#include "vmkit/CollectionRV.h"

#include "debug.h"

namespace vmkit {

void CollectionRV::another_mark() {
  vmkit::Thread* th = vmkit::Thread::get();
  assert(th->getLastSP() != 0);
  assert(nbJoined < th->MyVM->numberOfThreads);
  nbJoined++;
  if (nbJoined == th->MyVM->numberOfThreads) {
    condInitiator.broadcast();
  }
}

void CollectionRV::waitEndOfRV() {
  vmkit::Thread* th = vmkit::Thread::get();
  assert(th->getLastSP() != 0);

  while (th->doYield) {
    condEndRV.wait(&_lockRV);
  }
}

void CollectionRV::waitRV() {
  vmkit::Thread* self = vmkit::Thread::get(); 
  // Add myself.
  nbJoined++;

  //fprintf(stderr, "waitRV: %d from %d\n", nbJoined, self->MyVM->numberOfThreads);

  while (nbJoined != self->MyVM->numberOfThreads) {
	  vmkit::Thread* cur = self;
	  //fprintf(stderr, "Wasting time : ");
	    for (cur = (vmkit::Thread*)self->next(); cur != self;
	         cur = (vmkit::Thread*)cur->next()) {
//	      if (!cur->getLastSP() && cur != self) {
//	        fprintf(stderr, "%p,", cur);
//	      }
	    }
//	    fprintf(stderr, "\n");
    condInitiator.wait(&_lockRV);
  } 
}

void CooperativeCollectionRV::synchronize() {
  assert(nbJoined == 0);
  vmkit::Thread* self = vmkit::Thread::get();
  // Lock thread lock, so that we can traverse the thread list safely. This will
  // be released on finishRV.
  self->MyVM->threadLock.lock();

  vmkit::Thread* cur = self;
  assert(initiator == NULL);
  initiator = self;
  do {
    cur->doYield = true;
    assert(!cur->joinedRV);
    cur = (vmkit::Thread*)cur->next();
  } while (cur != self);
 
  // The CAS is not necessary but it does a memory barrier. 
  __sync_bool_compare_and_swap(&(self->joinedRV), false, true);

  // Lookup currently blocked threads.
  for (cur = (vmkit::Thread*)self->next(); cur != self; 
       cur = (vmkit::Thread*)cur->next()) {
    if (cur->getLastSP()) {
      nbJoined++;
      cur->joinedRV = true;
    }
  }
  
//  fprintf(stderr, "synchronize: %d from %d\n", nbJoined, self->MyVM->numberOfThreads);

  // And wait for other threads to finish.
  waitRV();

  // Unlock, so that threads in uncooperative code that go back to cooperative
  // code can set back their lastSP.
  unlockRV();
}

void CooperativeCollectionRV::join() {
  vmkit::Thread* th = vmkit::Thread::get();
  assert(th->doYield && "No yield");
  assert((th->getLastSP() == 0) && "SP present in cooperative code");

  th->inRV = true;
  
  lockRV();
  th->setLastSP(System::GetCallerAddress());
  th->joinedRV = true;
  another_mark();
  waitEndOfRV();
  th->setLastSP(0);
  unlockRV();
  
  th->inRV = false;
}

void CooperativeCollectionRV::joinBeforeUncooperative() {
  vmkit::Thread* th = vmkit::Thread::get();
  assert((th->getLastSP() != 0) &&
         "SP not set before entering uncooperative code");

  th->inRV = true;
  
  lockRV();
  if (th->doYield) {
    if (!th->joinedRV) {
      th->joinedRV = true;
      another_mark();
    }
    waitEndOfRV();
  }
  unlockRV();

  th->inRV = false;
}

void CooperativeCollectionRV::joinAfterUncooperative(word_t SP) {
  vmkit::Thread* th = vmkit::Thread::get();
  assert((th->getLastSP() == 0) &&
         "SP set after entering uncooperative code");

  th->inRV = true;

  lockRV();
  if (th->doYield) {
    th->setLastSP(SP);
    if (!th->joinedRV) {
      th->joinedRV = true;
      another_mark();
    }
    waitEndOfRV();
    th->setLastSP(0);
  }
  unlockRV();

  th->inRV = false;
}

extern "C" void conditionalSafePoint() {
  vmkit::Thread* th = vmkit::Thread::get();
  th->MyVM->rendezvous.join();
}

void CooperativeCollectionRV::finishRV() {
  lockRV();
  
  assert(vmkit::Thread::get() == initiator);
  vmkit::Thread* cur = initiator;
  do {
    assert(cur->doYield && "Inconsistent state");
    assert(cur->joinedRV && "Inconsistent state");
    cur->doYield = false;
    cur->joinedRV = false;
    cur = (vmkit::Thread*)cur->next();
  } while (cur != initiator);

  assert(nbJoined == initiator->MyVM->numberOfThreads && "Inconsistent state");
  nbJoined = 0;
  initiator->MyVM->threadLock.unlock();
  condEndRV.broadcast();
  initiator = NULL;
  unlockRV();
  vmkit::Thread::get()->inRV = false;
}

void CooperativeCollectionRV::addThread(Thread* th) {
  // Nothing to do.
}

}
