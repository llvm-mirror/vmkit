//===------------- Main.cpp - Simple execution of J3 ----------------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC.h"
#include "mvm/JIT.h"
#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "j3/JavaJITCompiler.h"
#include "../../lib/J3/VMCore/JnjvmClassLoader.h"
#include "../../lib/J3/VMCore/Jnjvm.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

extern llvm::cl::opt<bool> StandardCompileOpts;

using namespace j3;
using namespace mvm;

int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;  

  // Initialize base components.  
  mvm::BumpPtrAllocator Allocator;
	mvm::VMKit* vmkit = new(Allocator, "VMKit") mvm::VMKit(Allocator);
  
  // Tell the compiler to run all optimizations.
  StandardCompileOpts = true;
 
  // Create the allocator that will allocate the bootstrap loader and the JVM.
  JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
  Jnjvm* vm = new(Allocator, "VM") Jnjvm(Allocator, vmkit, Comp, true);
 
  // Run the application. 
  vm->runApplication(argc, argv);
  vmkit->waitNonDamonThreads();
  exit(0);

  // Destroy everyone.
  // vm->~Jnjvm();
  // loader->~JnjvmBootstrapLoader();

  return 0;
}
