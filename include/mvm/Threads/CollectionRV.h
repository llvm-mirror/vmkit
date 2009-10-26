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
  
  /// stackLock - Stack lock for synchronization.
  LockNormal _stackLock;         
  
  /// stackCond - Condition for unlocking other tasks (write protect).
  Cond stackCond;

  /// collectionCond - Condition for unblocking the collector.
  Cond collectionCond;

  /// nbCollected - Number of threads collected.
  unsigned nbCollected;
  
  /// currentCollector - The initiating thread for collection. Don't
  /// synchonize this one.
  mvm::Thread* currentCollector;  
  
  /// cooperative - Is the rendez-vous cooperative?
  bool cooperative;


  /// rendezvousNb - The identifier of the rendez-vous.
  ///
  unsigned rendezvousNb;
  
public:
 
  mvm::Thread* getCurrentCollector() {
    return currentCollector;
  }

  CollectionRV() {
    rendezvousNb = 0;
    nbCollected = 0;
    currentCollector = 0;
#ifdef WITH_LLVM_GCC
    cooperative = true;
#else
    cooperative = false;
#endif
  }
 
  bool isCooperative() { return cooperative; }

  void stackLock() { _stackLock.lock(); }
  void stackUnlock() { _stackLock.unlock(); }

  void waitStacks();
  void waitCollection();
 
  void collectionFinished() {
    if (cooperative) {
      // We lock here to make sure no threads previously blocked in native
      // will join the collection and never go back to running code.
      stackLock();
      mvm::Thread* cur = currentCollector;
      do {
        cur->doYield = false;
        cur = (mvm::Thread*)cur->next();
      } while (cur != currentCollector);
      rendezvousNb++;
      collectionCond.broadcast();
      stackUnlock();
    } else {
      rendezvousNb++;
      collectionCond.broadcast();
    }
    currentCollector->inGC = false;
  }
  
  void collectorGo() { stackCond.broadcast(); }

  void another_mark() { nbCollected++; }

  void synchronize();

  void traceForeignThreadStack(mvm::Thread* th);
  void traceThreadStack();

};

}

#endif
