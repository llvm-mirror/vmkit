//===--------------- gcinit.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <signal.h>
#include "MvmGC.h"

using namespace mvm;

static const size_t  def_collect_freq_auto = 64*1024*1024;
static const size_t def_collect_freq_maybe = 64*1024*1024;

#if defined(__MACH__)
# define SIGGC  SIGXCPU
#else
# define SIGGC  SIGPWR
#endif

int Collector::siggc() {
  return SIGGC;
}

void Collector::initialise() {
 
  used_nodes = new GCChunkNode();
  unused_nodes = new GCChunkNode();
#ifdef HAVE_PTHREAD
  threads = new GCThread();
#endif
  
  struct sigaction sa;
  sigset_t mask;

#ifdef HAVE_PTHREAD
  //on_fatal = unlock_dont_recovery;
#endif

  sigaction(SIGGC, 0, &sa);
  sigfillset(&mask);
  sa.sa_mask = mask;
  sa.sa_handler = siggc_handler;
  sa.sa_flags |= SA_RESTART;
  sigaction(SIGGC, &sa, 0);

  allocator = new GCAllocator();
   
  used_nodes->alone();
  unused_nodes->alone();

  current_mark = 0;
  status = stat_alloc;

  _collect_freq_auto = def_collect_freq_auto;
  _collect_freq_maybe = def_collect_freq_maybe;
  
  _since_last_collection = _collect_freq_auto;

  _enable_auto = 1;
  _enable_collection = 1;
  _enable_maybe = 1;

}

void Collector::destroy() {
  delete allocator;
  allocator = 0;
}

#ifdef HAVE_PTHREAD
void Collector::inject_my_thread(mvm::Thread* th) {
  threads->inject(th);
}
#endif

