//===---------------- Threads.h - Micro-vm threads ------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_THREAD_H
#define MVM_THREAD_H

#include "types.h"

#include "MvmGC.h"
#include "mvm/Threads/Key.h"


class Collector;

namespace mvm {

class Thread : public gc {
public:
  static void yield(void);
  static void yield(unsigned int *);
  static int self(void);
  static void initialise(void);
  static int kill(int tid, int signo);
  static void exit(int value);
  static int start(int *tid, int (*fct)(void *), void *arg);
  
  static mvm::Key<Thread>* threadKey;
  Collector* GC;
  void* baseSP;
  uint32 threadID;
  static Thread* get() {
    return (Thread*)Thread::threadKey->get();
  }

};


} // end namespace mvm
#endif // MVM_THREAD_H
