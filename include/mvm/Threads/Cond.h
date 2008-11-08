//===---------------- Cond.h - Threads conditions -------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_COND_H
#define MVM_COND_H

#include <cstdlib>
#include <pthread.h>

namespace mvm {

class Lock;

class Cond {
  pthread_cond_t internalCond;
public:
  
  Cond();
  ~Cond();
  void broadcast(void);
  void wait(Lock *l);
  int timedWait(Lock *l, timeval *tv);
  void signal(void);
};

} // end namespace mvm

#endif // MVM_COND_H
