//===------------ Main.cpp - Simple execution of N3 -----------------------===//
//
//                            N3
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VMLet.h"
#include "mvm/Threads/Thread.h"


#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "llvm/Support/ManagedStatic.h"

using namespace mvm;


typedef int (*boot_t)(int, char**, char**);

static void clearSignals(void) {
  signal(SIGINT,  SIG_DFL);
  signal(SIGILL,  SIG_DFL);
#if !defined(WIN32)
  signal(SIGIOT,  SIG_DFL);
  signal(SIGBUS,  SIG_DFL);
#endif
  signal(SIGSEGV, SIG_DFL);
}

extern "C" int boot();
extern "C" int start_app(int, char**);

#include "VMCore/CLIJit.h"

void handler2(int n, void *context) {
  printf("[%d] crashed\n", (int)mvm::Thread::self());
  n3::CLIJit::printBacktrace();
  assert(0);
}

void handler(int n, siginfo_t *_info, void *context) {
  printf("[%d] crashed\n", (int)mvm::Thread::self());
  n3::CLIJit::printBacktrace();
  assert(0);
}

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
  int base;
  
  struct sigaction sa;

  sigaction(SIGINT, 0, &sa);
  sa.sa_sigaction = handler;
  sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
  sigaction(SIGINT, &sa, 0);
  
  sigaction(SIGILL, 0, &sa);
  sa.sa_sigaction = handler;
  sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
  sigaction(SIGILL, &sa, 0);
  
  
  VMLet::register_sigsegv_handler(handler2);

  Object::initialise(&base);
  VMLet::initialise();
  boot();
  start_app(argc, argv);

  clearSignals();
    
  return 0;
}
