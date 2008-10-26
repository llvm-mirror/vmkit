//===--------- Main.cpp - Simple execution of JnJVM -----------------------===//
//
//                            JnJVM
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "llvm/Support/ManagedStatic.h"

using namespace mvm;

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
  int base;
    
  MvmModule::initialise();
  Object::initialise();
  Thread::initialise();
  Collector::initialise(0, &base);
  
  CompilationUnit* CU = VirtualMachine::initialiseJVM();
  VirtualMachine* vm = VirtualMachine::createJVM(CU);
  vm->runApplication(argc, argv);

  return 0;
}
