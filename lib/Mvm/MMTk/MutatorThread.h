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
  
  /// realRoutine - The function to invoke when the thread starts.
  ///
  void (*realRoutine)(mvm::Thread*);
 

  static uint32_t MMTkMutatorSize;
  static uint32_t MMTkCollectorSize;

  typedef void (*MMTkInitType)(uintptr_t);
  typedef void (*MMTkInitIntType)(uintptr_t, int32_t);
  static MMTkInitType    MutatorInit;
  static MMTkInitIntType MutatorCallInit;
  static MMTkInitType    MutatorCallDeinit;
  static MMTkInitType    CollectorInit;

  static VirtualTable* MutatorVT;
  static VirtualTable* CollectorVT;

  static void init(Thread* _th) {
    MutatorThread* th = (MutatorThread*)_th;
    th->MutatorContext =
      (uintptr_t)th->Allocator.Allocate(MMTkMutatorSize, "Mutator");
    ((VirtualTable**)th->MutatorContext)[0] = MutatorVT;
    MutatorInit(th->MutatorContext);
    MutatorCallInit(th->MutatorContext, (int32_t)_th->getThreadID());
    th->realRoutine(_th);
    MutatorCallDeinit(th->MutatorContext);
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
