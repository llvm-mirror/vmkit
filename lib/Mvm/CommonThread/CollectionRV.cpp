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
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/CollectionRV.h"
#include "mvm/VMKit.h"
#include "mvm/GC.h"

#include "debug.h"

using namespace mvm;

void CollectionRV::addThread(mvm::Thread* th) {
	threadLock.lock();
	numberOfThreads++;
	if (th != oneThread) {
		if (oneThread) th->append(oneThread);
		else oneThread = th;
	}
	threadLock.unlock();
}
  
void CollectionRV::removeThread(mvm::Thread* th) {
	threadLock.lock();
	numberOfThreads--;
	if (oneThread == th) oneThread = (Thread*)th->next();
	th->remove();
	if (!numberOfThreads) oneThread = 0;
	threadLock.unlock();
}

void CollectionRV::another_mark() {
  assert(th->getLastSP() != NULL);
  assert(nbJoined < th->MyVM->NumberOfThreads);
  nbJoined++;
  if (nbJoined == numberOfThreads) {
    condInitiator.broadcast();
  }
}

void CollectionRV::waitEndOfRV() {
  mvm::Thread* th = mvm::Thread::get();
  assert(th->getLastSP() != NULL);

  while (th->doYield) {
    condEndRV.wait(&_lockRV);
  }
}

void CollectionRV::waitRV() {
  // Add myself.
  nbJoined++;

  while (nbJoined != numberOfThreads) {
    condInitiator.wait(&_lockRV);
  } 
}

void CooperativeCollectionRV::synchronize() {
  assert(nbJoined == 0);
  mvm::Thread* self = mvm::Thread::get();
  // Lock thread lock, so that we can traverse the thread list safely. This will
  // be released on finishRV.
 threadLock.lock();

  mvm::Thread* cur = self;
  do {
    assert(!cur->doYield);
    cur->doYield = true;
    assert(!cur->joinedRV);
    cur = (mvm::Thread*)cur->next();
  } while (cur != self);
 
  // The CAS is not necessary but it does a memory barrier. 
  __sync_bool_compare_and_swap(&(self->joinedRV), false, true);

  // Lookup currently blocked threads.
  for (cur = (mvm::Thread*)self->next(); cur != self; 
       cur = (mvm::Thread*)cur->next()) {
    if (cur->getLastSP()) {
      nbJoined++;
      cur->joinedRV = true;
    }
  }
  
  // And wait for other threads to finish.
  waitRV();

  // Unlock, so that threads in uncooperative code that go back to cooperative
  // code can set back their lastSP.
  unlockRV();
}


#if defined(__MACH__)
# define SIGGC  SIGXCPU
#else
# define SIGGC  SIGPWR
#endif

void UncooperativeCollectionRV::synchronize() { 
  assert(nbJoined == 0);
  mvm::Thread* self = mvm::Thread::get();
  // Lock thread lock, so that we can traverse the thread list safely. This will
  // be released on finishRV.
  threadLock.lock();
  
  for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
       cur = (mvm::Thread*)cur->next()) {
    int res = cur->kill(SIGGC);
    assert(!res && "Error on kill");
  }
  
  // And wait for other threads to finish.
  waitRV();

  // Unlock, so that threads in uncooperative code that go back to cooperative
  // code can set back their lastSP.
  unlockRV();
}


void UncooperativeCollectionRV::join() {
  mvm::Thread* th = mvm::Thread::get();
  th->inRV = true;

  lockRV();
  void* old = th->getLastSP();
  th->setLastSP(FRAME_PTR());
  another_mark();
  waitEndOfRV();
  th->setLastSP(old);
  unlockRV();

  th->inRV = false;
}

void CooperativeCollectionRV::join() {
  mvm::Thread* th = mvm::Thread::get();
  assert(th->doYield && "No yield");
  assert((th->getLastSP() == NULL) && "SP present in cooperative code");

  th->inRV = true;
  
  lockRV();
  th->setLastSP(FRAME_PTR());
  th->joinedRV = true;
  another_mark();
  waitEndOfRV();
  th->setLastSP(0);
  unlockRV();
  
  th->inRV = false;
}

void CooperativeCollectionRV::joinBeforeUncooperative() {
  mvm::Thread* th = mvm::Thread::get();
  assert((th->getLastSP() != NULL) &&
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

void CooperativeCollectionRV::joinAfterUncooperative(void* SP) {
  mvm::Thread* th = mvm::Thread::get();
  assert((th->getLastSP() == NULL) &&
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
    th->setLastSP(NULL);
  }
  unlockRV();

  th->inRV = false;
}

extern "C" void conditionalSafePoint() {
  mvm::Thread* th = mvm::Thread::get();
  th->vmkit()->rendezvous.join();
}

void CooperativeCollectionRV::finishRV() {
  lockRV();
    
  mvm::Thread* initiator = mvm::Thread::get();
  mvm::Thread* cur = initiator;
  do {
    assert(cur->doYield && "Inconsistent state");
    assert(cur->joinedRV && "Inconsistent state");
    cur->doYield = false;
    cur->joinedRV = false;
    cur = (mvm::Thread*)cur->next();
  } while (cur != initiator);

  assert(nbJoined == initiator->MyVM->NumberOfThreads && "Inconsistent state");
  nbJoined = 0;
  threadLock.unlock();
  condEndRV.broadcast();
  unlockRV();
  initiator->inRV = false;
}

void CooperativeCollectionRV::prepareForJoin() {
	/// nothing to do
}

void UncooperativeCollectionRV::finishRV() {
  lockRV();
  mvm::Thread* initiator = mvm::Thread::get();
  assert(nbJoined == initiator->MyVM->NumberOfThreads && "Inconsistent state");
  nbJoined = 0;
  threadLock.unlock();
  condEndRV.broadcast();
  unlockRV();
  initiator->inRV = false;
}

void UncooperativeCollectionRV::joinAfterUncooperative(void* SP) {
  UNREACHABLE();
}

void UncooperativeCollectionRV::joinBeforeUncooperative() {
  UNREACHABLE();
}

static void siggcHandler(int) {
  mvm::Thread* th = mvm::Thread::get();
  th->vmkit()->rendezvous.join();
}

void UncooperativeCollectionRV::prepareForJoin() {
  // Set the SIGGC handler for uncooperative rendezvous.
  struct sigaction sa;
  sigset_t mask;
  sigaction(SIGGC, 0, &sa);
  sigfillset(&mask);
  sa.sa_mask = mask;
  sa.sa_handler = siggcHandler;
  sa.sa_flags |= SA_RESTART;
  sigaction(SIGGC, &sa, NULL);
  
  if (nbJoined != 0) {
    // In uncooperative mode, we may have missed a signal.
    join();
  }
}
