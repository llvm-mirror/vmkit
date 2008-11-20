//===--------- Launcher.cpp - Launch command line -------------------------===//
//
//                            JnJVM
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <dlfcn.h>

#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"


#include "MvmGC.h"
#include "mvm/Config/config.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "CommandLine.h"

enum VMType {
  Interactive, RunJava, RunNet
};

static llvm::cl::opt<VMType> VMToRun(llvm::cl::desc("Choose VM to run:"),
  llvm::cl::values(
    clEnumValN(Interactive , "i", "Run in interactive mode"),
    clEnumValN(RunJava , "java", "Run the JVM"),
    clEnumValN(RunNet, "net", "Run the CLI VM"),
   clEnumValEnd));

static llvm::cl::opt<bool> Fast("fast", 
                     cl::desc("Generate code quickly, "
                              "potentially sacrificing code quality"),
                     cl::init(false));

int found(char** argv, int argc, const char* name) {
  int i = 1;
  for (; i < argc; i++) {
    if (!(strcmp(name, argv[i]))) return i + 1;
  }
  return 0;
}

int main(int argc, char** argv) {
  llvm::llvm_shutdown_obj X;
  
  int pos = found(argv, argc, "-java");
  if (pos) {
    llvm::cl::ParseCommandLineOptions(pos, argv);
  } else {
    pos = found(argv, argc, "-net");
    if (pos) {
      llvm::cl::ParseCommandLineOptions(pos, argv);
    } else {
      llvm::cl::ParseCommandLineOptions(argc, argv);
    }
  }
  
  mvm::MvmModule::initialise(Fast);
  mvm::Object::initialise();
  Collector::initialise(0);
  
  if (VMToRun == RunJava) {
#if WITH_JNJVM
    mvm::CompilationUnit* CU = mvm::VirtualMachine::initialiseJVM();
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(CU);
    vm->runApplication(argc, argv);
    vm->waitForExit();
#endif
  } else if (VMToRun == RunNet) {
#if WITH_N3
    mvm::CompilationUnit* CU = mvm::VirtualMachine::initialiseCLIVM();
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createCLIVM(CU);
    vm->runApplication(argc, argv);
    vm->waitForExit();
#endif
  } else {
    mvm::CommandLine MyCl;
#if WITH_JNJVM
    mvm::CompilationUnit* JVMCompiler = 
      mvm::VirtualMachine::initialiseJVM();
    MyCl.vmlets["java"] = (mvm::VirtualMachine::createJVM);
    MyCl.compilers["java"] = JVMCompiler;
#endif
#if WITH_N3
    mvm::CompilationUnit* CLICompiler = 
      mvm::VirtualMachine::initialiseCLIVM();
    MyCl.vmlets["net"] = (mvm::VirtualMachine::createCLIVM);
    MyCl.compilers["net"] = CLICompiler;
#endif
    MyCl.start();
  }

  return 0;
}
