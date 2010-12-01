//===--------- Main.cpp - Simple execution of JnJVM -----------------------===//
//
//                            JnJVM
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "llvm/Support/ManagedStatic.h"

using namespace mvm;

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
    
  MvmModule::initialise();
  Collector::initialise();

  VirtualMachine::initialiseCLIVM();
  VirtualMachine* vm = VirtualMachine::createCLIVM();
  vm->runApplication(argc, argv);
  vm->waitForExit();

  return 0;
}
