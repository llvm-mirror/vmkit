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

  
public:

  GCThread() {}
  
  void lock()   { _globalLock.lock(); }
  void unlock() { _globalLock.unlock(); }

};

}
#endif
