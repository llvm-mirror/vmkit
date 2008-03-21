//===--------- Main.cpp - Simple execution of JnJVM -----------------------===//
//
//                            JnJVM
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VMLet.h"


#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "llvm/Support/ManagedStatic.h"

#include "mvm/Threads/Thread.h"

using namespace mvm;


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
extern "C" int ClasspathBoot(int, char**, char**);

#include "VMCore/JavaJIT.h"

void handler2(int n, void *context) {
  printf("[%d] crashed\n", (int)mvm::Thread::self());
  jnjvm::JavaJIT::printBacktrace();
  assert(0);
}

void handler(int n, siginfo_t *_info, void *context) {
  printf("[%d] crashed\n", (int)mvm::Thread::self());
  jnjvm::JavaJIT::printBacktrace();
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
  ClasspathBoot(argc, argv, envp);
  boot();
  start_app(argc, argv);

  clearSignals();
    
  return 0;
}
