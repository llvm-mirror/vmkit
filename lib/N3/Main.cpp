//===------------ Main.cpp - Simple execution of N3 -----------------------===//
//
//                            N3
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Thread.h"

#include "llvm/Support/ManagedStatic.h"

using namespace mvm;


extern "C" int boot();
extern "C" int start_app(int, char**);


int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
  int base;
  
  Thread::initialise();
  jit::initialise();
  Object::initialise();
  Collector::initialise(0, &base);
  boot();
  start_app(argc, argv);

  return 0;
}
