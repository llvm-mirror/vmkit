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

  static size_t MutatorSize;
  static size_t CollectorSize;

  static void (*CollectorInit)(uintptr_t);
  static void (*MutatorInit)(uintptr_t);

  MutatorThread() {
    MutatorContext = Allocator.Allocate(MutatorSize);
    MutatorInit(MutatorContext);
    CollectorContext = Allocator.Allocate(CollectorSize);
    CollectorInit(CollectorContext);
  }

};

}

#endif
