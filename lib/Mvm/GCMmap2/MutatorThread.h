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

#include "mvm/Threads/Thread.h"

namespace mvm {

class MutatorThread : public mvm::Thread {
public:
  MutatorThread(VMKit* vmkit) : mvm::Thread(vmkit) {}
};

}

#endif
