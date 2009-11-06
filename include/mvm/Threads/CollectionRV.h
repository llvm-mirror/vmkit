//===---------- CollectionRV.h - Rendez-vous for garbage collection -------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _MVM_COLLECTIONRV_H_
#define _MVM_COLLECTIONRV_H_

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

namespace mvm {

class CollectionRV {
  
  /// _lockRV - Lock for synchronization.
  LockNormal _lockRV;         
  
  /// condEndRV - Condition for unlocking other tasks (write protect).
  Cond condEndRV;

  /// collectionCond - Condition for unblocking the initator.
  Cond condInitiator;

  /// nbJoined - Number of threads that joined the rendezvous.
  unsigned nbJoined;
  
  /// cooperative - Is the rendez-vous cooperative?
  bool cooperative;

  /// rendezvousNb - The identifier of the rendez-vous.
  unsigned rendezvousNb;
  
public:
 
  CollectionRV() {
    rendezvousNb = 0;
    nbJoined = 0;
#ifdef WITH_LLVM_GCC
    cooperative = true;
#else
    cooperative = false;
#endif
  }
 
  bool isCooperative() { return cooperative; }

  void lockRV() { _lockRV.lock(); }
  void unlockRV() { _lockRV.unlock(); }

  void waitEndOfRV();
  void waitRV();
  
  void startRV() {
    mvm::Thread::get()->inRV = true;
    lockRV();
  }

  void cancelRV() {
    mvm::Thread::get()->inRV = false;
    unlockRV();
  }
 
  void finishRV();
  
  void collectorGo() { condInitiator.broadcast(); }

  void another_mark() { nbJoined++; }

  void synchronize();

  void join();

  unsigned getNumber() { return rendezvousNb; }
};

}

#endif
