//===--------- MutatorThread.h - Thread for GC ----------------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef VMKIT_MUTATOR_THREAD_H
#define VMKIT_MUTATOR_THREAD_H

#include "vmkit/Allocator.h"
#include "vmkit/Thread.h"

namespace vmkit {

class MutatorThread : public vmkit::Thread {
public:
  MutatorThread() : vmkit::Thread() {
    MutatorContext = 0;
    CollectionAttempts = 0;
  }
  vmkit::ThreadAllocator Allocator;
  word_t MutatorContext;
  
  /// realRoutine - The function to invoke when the thread starts.
  ///
  void (*realRoutine)(vmkit::Thread*);

  uint32_t CollectionAttempts;

  static void init(Thread* _th);

  static MutatorThread* get() {
    return (MutatorThread*)vmkit::Thread::get();
  }

  virtual int start(void (*fct)(vmkit::Thread*)) {
    realRoutine = fct;
    routine = init;
    return Thread::start(init);
  }
};

}

#endif
