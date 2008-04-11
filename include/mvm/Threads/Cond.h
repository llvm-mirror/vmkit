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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mvm/Threads/Locks.h"

#include <cstdlib>

namespace mvm {

class Cond {
  unsigned int    no_barrier;
  unsigned int    go; 
  unsigned int    n_wait;
public:
  static Cond *allocCond(void);
  void broadcast(void);
  void wait(Lock *l);
  int timed_wait(Lock *l, timeval *tv);
  void signal(void);
};

} // end namespace mvm

#endif // MVM_COND_H
