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

#include "mvm/GC/GC.h"

namespace mvm {
class Thread {
public:
  static void yield(void);
  static void yield(unsigned int *);
  static int self(void);
  static void initialise(void);
  static int kill(int tid, int signo);
  static void exit(int value);
  static int start(int *tid, int (*fct)(void *), void *arg);

};


} // end namespace mvm
#endif // MVM_THREAD_H
