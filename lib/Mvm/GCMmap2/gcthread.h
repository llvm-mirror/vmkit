//===--------------- gcthread.h - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_THREAD_H_
#define _GC_THREAD_H_

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

namespace mvm {

class GCThread {
  /// _globalLock - Global lock for gcmalloc.
  SpinLock _globalLock;

  /// _stackLock - Stack lock for synchronization.
  LockNormal _stackLock;         
  
  /// _stackCond - Condition for unlocking other tasks (write protect).
  Cond _stackCond;

  /// _collectionCond - Condition for unblocking the collector.
  Cond _collectionCond;

  /// _nb_threads - Number of active threads.
  unsigned int _nb_threads;

  /// _nb_collected - Number of threads collected.
  unsigned int _nb_collected;
  
  /// current_collector - The initiating thread for collection. Don't
  /// synchonize this one.
  mvm::Thread* current_collector;  

  
public:
  mvm::Thread* base;
  bool cooperative;
 
  mvm::Thread* getCurrentCollector() {
    return current_collector;
  }

  GCThread() {
    _nb_threads = 0;
    _nb_collected = 0;
    current_collector = 0;
    base = 0;
#ifdef WITH_LLVM_GCC
    cooperative = true;
#else
    cooperative = false;
#endif
  }
  
  inline unsigned int get_nb_threads() {
    return _nb_threads;
  }
  inline void lock()   { _globalLock.lock(); }
  inline void unlock() { _globalLock.unlock(); }

  inline void stackLock() { _stackLock.lock(); }
  inline void stackUnlock() { _stackLock.unlock(); }

  void        waitStacks();
  void        waitCollection();
 
  inline void collectionFinished() {
    if (cooperative) {
      // We lock here to make sure no threads previously blocked in native
      // will join the collection and never go back to running code.
      stackLock();
      mvm::Thread* cur = current_collector;
      do {
        cur->doYield = false;
        cur = (mvm::Thread*)cur->next();
      } while (cur != current_collector);
      _collectionCond.broadcast();
      stackUnlock();
    } else {
      _collectionCond.broadcast();
    }
    current_collector->inGC = false;
  }
  
  inline void collectorGo() { _stackCond.broadcast(); }

  inline void cancel() {
    // all stacks have been collected
    _nb_collected = _nb_threads;
    // unblock all threads in stack collection
    collectorGo();
    // unblock mutators
    collectionFinished();         
  }

  inline void another_mark() { _nb_collected++; }

  void synchronize();

  inline void remove(mvm::Thread* th) {
    lock();
    th->remove();
    _nb_threads--;
    if (!_nb_threads) base = 0;
#ifdef SERVICE
    th->MyVM->numThreads--;
#endif
    unlock();
  }

  inline void inject(mvm::Thread* th) { 
    lock(); 
#ifdef SERVICE
    if (th->MyVM->numThreads + 1 > th->MyVM->threadLimit) {
      unlock();
      th->MyVM->stopService();
    }
    th->MyVM->numThreads++;
#endif
    if (base)
      th->append(base);
    else
      base = th;
    _nb_threads++;
    unlock();
  }
};

}
#endif
