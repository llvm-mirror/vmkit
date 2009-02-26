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

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

#include "llvm/Support/ManagedStatic.h"


using namespace jnjvm;
using namespace llvm;
using namespace mvm;

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  
    
  MvmModule::initialise();
  Object::initialise();
  Collector::initialise(0);
 
  CompilationUnit* CU = VirtualMachine::initialiseJVM();
  CU->TheModule = new JnjvmModuleJIT("JITModule");
  CU->TheModuleProvider = new JnjvmModuleProvider((JnjvmModule*)CU->TheModule);
  CU->AddStandardCompilePasses();
  VirtualMachine* vm = VirtualMachine::createJVM(CU);
  vm->runApplication(argc, argv);
  vm->waitForExit();

  return 0;
}
