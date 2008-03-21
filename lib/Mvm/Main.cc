//===--------------- Main.cc - Execution of the mvm -----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Thread.h"
#include "mvm/Sigsegv.h"
#include "mvm/VMLet.h"


#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

#include "CommandLine.h"

using namespace mvm;


typedef int (*boot)(int, char**, char**);

static void clearSignals(void) {
  signal(SIGINT,  SIG_DFL);
  signal(SIGILL,  SIG_DFL);
#if !defined(WIN32)
  signal(SIGIOT,  SIG_DFL);
  signal(SIGBUS,  SIG_DFL);
#endif
  signal(SIGSEGV, SIG_DFL);
}

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
  int base;
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    " VMKit: a virtual machine launcher\n");
  Object::initialise(&base);
  VMLet::initialise();
  
  CommandLine cl;
  cl.start();
   
  clearSignals();
	Thread::exit(0);
    
  return 0;
}
