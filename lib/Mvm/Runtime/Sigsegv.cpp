//===----------- Sigsegv.cc - Sigsegv default handling --------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "MvmGC.h"
#include "mvm/Threads/Thread.h"

#include <csignal>
#include <cstdio>

using namespace mvm;

void (*client_sigsegv_handler)(int, void *) = 0;

#if defined(__MACH__) && defined(__i386__)
#include "ucontext.h"
#endif

void sigsegv_handler(int n, siginfo_t *_info, void *context) {
  void *addr = _info->si_addr;
#if defined(__i386__)
  struct frame {
    struct frame *caller;
    void         *ip;
  };
  
  /* my frame */
  struct frame *fp;
  /* get it */
  asm ("mov %%ebp, %0" : "=&r"(fp));
  /* my caller */
  struct frame *caller = fp->caller; 
  /* preserve my caller if I return from the handler */
  void *caller_ip = caller->ip; 

#if defined(__MACH__)
  //.gregs[REG_EIP]; /* just like it's on the stack.. */
  caller->ip = (void *)((ucontext_t*)context)->uc_mcontext->__ss.__eip;
#else
  /* just like it's on the stack... */
  caller->ip = (void *)((ucontext_t*)context)->uc_mcontext.gregs[REG_EIP]; 
#endif
#endif
	
  /* Free the GC if it sisgegv'd. No other collection is possible */
  Collector::die_if_sigsegv_occured_during_collection(addr);

  //	sys_exit(0);
  if(client_sigsegv_handler)
    client_sigsegv_handler(n, addr);
  else
    signal(SIGSEGV, SIG_DFL);
	
#if defined(__i386__)
  caller->ip = caller_ip; /* restore the caller ip */
#endif
}
