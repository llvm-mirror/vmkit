//===--------------- gcinit.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <signal.h>
#include "gccollector.h"

using namespace mvm;

static const size_t  def_collect_freq_auto = 4*1024*1024;
static const size_t def_collect_freq_maybe = 512*1024;

#if defined(__MACH__)
# define SIGGC  SIGXCPU
#else
# define SIGGC  SIGPWR
#endif

int GCCollector::siggc() {
  return SIGGC;
}

void GCCollector::initialise(Collector::markerFn marker) {
 
#ifdef SERVICE_GC
  if (this == bootstrapGC) {
#endif
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
   
  _marker = marker;

  used_nodes->alone();

  current_mark = 0;
  status = stat_alloc;
#ifdef SERVICE_GC
  }
#endif

  _collect_freq_auto = def_collect_freq_auto;
  _collect_freq_maybe = def_collect_freq_maybe;
  
  _since_last_collection = _collect_freq_auto;

  _enable_auto = 1;
  _enable_collection = 1;
  _enable_maybe = 1;

}

void GCCollector::destroy() {
  delete allocator;
  allocator = 0;
}


static void *get_curr_fp(void)
{
  register void *fp;
  asm(
#  if defined(__ppc__) || defined(__PPC__)
#   if defined(__MACH__)
      "mr  %0, r1"
#   else
      "mr  %0, 1"
#   endif
#  elif defined(__i386__)
      "movl  %%ebp, %0"
# elif defined(__amd64__)
      "movq    %%rbp, %0"
#  else
#   error:
#   error: I do not know how to read the frame pointer on this machine
#   error:
#  endif
      :"=r"(fp):);
  return fp;
}

static void *get_base_sp(void)
{
  void *fp= 0;
  for (fp= get_curr_fp();  (*(void **)fp);  fp= *(void **)fp);
  return fp;
}

#ifdef HAVE_PTHREAD
void GCCollector::inject_my_thread(void *base_sp) {
   if(!base_sp)
     base_sp = get_base_sp();
  threads->inject(base_sp, current_mark);
}
#endif

