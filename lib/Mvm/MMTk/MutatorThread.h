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
  
  /// realRoutine - The function to invoke when the thread starts.
  ///
  void (*realRoutine)(mvm::Thread*);
 

  static uint32_t MMTkMutatorSize;
  static uint32_t MMTkCollectorSize;

  typedef void (*MMTkInitType)(uintptr_t);
  static MMTkInitType MutatorInit;
  static MMTkInitType CollectorInit;

  static VirtualTable* MutatorVT;
  static VirtualTable* CollectorVT;

  static void init(Thread* _th) {
    MutatorThread* th = (MutatorThread*)_th;
    th->MutatorContext =
      (uintptr_t)th->Allocator.Allocate(MMTkMutatorSize, "Mutator");
    ((VirtualTable**)th->MutatorContext)[0] = MutatorVT;
    MutatorInit(th->MutatorContext);
    th->CollectorContext = 
      (uintptr_t)th->Allocator.Allocate(MMTkCollectorSize, "Collector");
    ((VirtualTable**)th->CollectorContext)[0] = CollectorVT;
    CollectorInit(th->CollectorContext);
    th->realRoutine(_th);
  }

  static MutatorThread* get() {
    return (MutatorThread*)mvm::Thread::get();
  }

  virtual int start(void (*fct)(mvm::Thread*)) {
    realRoutine = fct;
    routine = init;
    return Thread::start(init);
  }
};

}

#endif
