//===----------- Sigsegv.cc - Sigsegv default handling --------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "mvm/GC/GC.h"
#include "mvm/Sigsegv.h"
#include "mvm/VMLet.h"

#include <signal.h>
#include <stdio.h>

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

	struct frame *fp;	/* my frame */
  asm ("mov %%ebp, %0" : "=&r"(fp)); /* get it */
	struct frame *caller = fp->caller; /* my caller */
  /* preserve my caller if I return from the handler */
	void *caller_ip = caller->ip; 

# if defined(__MACH__)
  //.gregs[REG_EIP]; /* just like it's on the stack.. */
	caller->ip = (void *)((ucontext_t*)context)->uc_mcontext->__ss.__eip;
# else
  /* comme si c'était lui qui était sur la pile... */
	caller->ip = (void *)((ucontext_t*)context)->uc_mcontext.gregs[REG_EIP]; 
# endif
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

void VMLet::register_sigsegv_handler(void (*fct)(int, void *)) {
	struct sigaction sa;

	sigaction(SIGSEGV, 0, &sa);
	sa.sa_sigaction = sigsegv_handler;
	sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
	sigaction(SIGSEGV, &sa, 0);
	client_sigsegv_handler = fct;
}

