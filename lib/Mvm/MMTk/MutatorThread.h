//===--------- MutatorThread.h - Thread for GC ----------------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_MUTATOR_THREAD_H
#define MVM_MUTATOR_THREAD_H

#include "mvm/Allocator.h"
#include "mvm/Threads/Thread.h"

namespace mvm {

class MutatorThread : public mvm::Thread {
public:
  mvm::BumpPtrAllocator Allocator;
  uintptr_t MutatorContext;
  uintptr_t CollectorContext;

  static uint32_t MMTkMutatorSize;
  static uint32_t MMTkCollectorSize;

  static void (*MutatorInit)(uintptr_t);
  static void (*CollectorInit)(uintptr_t);


  MutatorThread() {
    MutatorContext = (uintptr_t)Allocator.Allocate(MMTkMutatorSize, "Mutator");
    MutatorInit(MutatorContext);
    CollectorContext = 
      (uintptr_t)Allocator.Allocate(MMTkCollectorSize, "Collector");
    CollectorInit(CollectorContext);
  }

  static MutatorThread* get() {
    return (MutatorThread*)mvm::Thread::get();
  }

};

}

#endif
