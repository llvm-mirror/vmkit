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

class GCLockRecovery : public LockNormal {
  gc_lock_recovery_fct_t _fct;
  int                    _args[8];
  
public:
  inline GCLockRecovery() { _fct = 0; }

  int verify_recall(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3,
                    int a4, int a5, int a6, int a7);

  inline void unlock_dont_recovery() { 
    if(selfOwner()) {
      LockNormal::unlock(); 
    }
  }

  inline void unlock() {
    if(_fct) {
      gc_lock_recovery_fct_t tmp = _fct;
      int l[8];
      l[0] = _args[0]; l[1] = _args[1]; l[2] = _args[2]; l[3] = _args[3];
      l[4] = _args[4]; l[5] = _args[5]; l[6] = _args[6]; l[7] = _args[7];
      _fct = 0;
      LockNormal::unlock();
      tmp(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
    } else
      LockNormal::unlock();
  }
};

class GCThread {
  /// _globalLoc - Global lock for gcmalloc.
  GCLockRecovery _globalLock;

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
  
  GCThread() {
    _nb_threads = 0;
    _nb_collected = 0;
    current_collector = 0;
    base = 0;
  }

  inline void lock()   { _globalLock.lock(); }
  inline void unlock() { _globalLock.unlock(); }
  inline void unlock_dont_recovery() { _globalLock.unlock_dont_recovery(); }
  inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2,
                      int a3, int a4, int a5, int a6, int a7) { 
    return _globalLock.verify_recall(fct, a0, a1, a2, a3, a4, a5, a6, a7);
  }

  inline void stackLock() { _stackLock.lock(); }
  inline void stackUnlock() { _stackLock.unlock(); }

  void        waitStacks();
  void        waitCollection();
  inline void collectionFinished() { _collectionCond.broadcast(); }
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
    unlock();
  }

  inline void inject(mvm::Thread* th) { 
    lock(); 
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
