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

extern "C" size_t MMTkMutatorSize;
extern "C" size_t MMTkCollectorSize;

extern "C" void JnJVM_org_j3_config_Selected_00024Mutator__0003Cinit_0003E__(uintptr_t);
extern "C" void JnJVM_org_j3_config_Selected_00024Collector__0003Cinit_0003E__(uintptr_t);


class MutatorThread : public mvm::Thread {
public:
  mvm::BumpPtrAllocator Allocator;
  uintptr_t MutatorContext;
  uintptr_t CollectorContext;

  MutatorThread() {
    MutatorContext = (uintptr_t)Allocator.Allocate(MMTkMutatorSize, "Mutator");
    JnJVM_org_j3_config_Selected_00024Mutator__0003Cinit_0003E__(MutatorContext);
    CollectorContext = (uintptr_t)Allocator.Allocate(MMTkCollectorSize, "Collector");
    JnJVM_org_j3_config_Selected_00024Collector__0003Cinit_0003E__(CollectorContext);
  }

  static MutatorThread* get() {
    return (MutatorThread*)mvm::Thread::get();
  }

};

}

#endif
