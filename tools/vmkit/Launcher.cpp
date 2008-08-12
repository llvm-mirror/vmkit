//===--------- Launcher.cpp - Launch command line -------------------------===//
//
//                            JnJVM
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <dlfcn.h>

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "CommandLine.h"

static llvm::cl::opt<bool> Java("java", llvm::cl::desc("Run the JVM"));
static llvm::cl::opt<bool> Net("net", llvm::cl::desc("Run the .Net VM"));

int found(char** argv, int argc, const char* name) {
  int i = 1;
  for (; i < argc; i++) {
    if (!(strcmp(name, argv[i]))) return i + 1;
  }
  return 0;
}

int main(int argc, char** argv) {
  llvm::llvm_shutdown_obj X;
  int base;
   
  mvm::jit::initialise();
  mvm::Object::initialise();
  mvm::Thread::initialise();
  Collector::initialise(0, &base);
  Collector::enable(0);
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
  
  if (Java) {
    mvm::VirtualMachine::initialiseJVM();
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM();
    vm->runApplication(argc, argv);
  } else if (Net) {
    mvm::VirtualMachine::initialiseCLIVM();
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createCLIVM();
    vm->runApplication(argc, argv);
  } else {
    mvm::VirtualMachine::initialiseJVM();
    mvm::VirtualMachine::initialiseCLIVM();
    mvm::VirtualMachine* bootstrapJVM = mvm::VirtualMachine::createJVM();
    mvm::VirtualMachine* bootstrapNet = mvm::VirtualMachine::createCLIVM();
    mvm::CommandLine MyCl;
    MyCl.vmlets["java"] = (mvm::VirtualMachine::createJVM);
    MyCl.vmlets["net"] = (mvm::VirtualMachine::createCLIVM);
    MyCl.start();
  }

  return 0;
}
