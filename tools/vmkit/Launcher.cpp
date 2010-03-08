//===--------- Launcher.cpp - Launch command line -------------------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/LinkAllPasses.h"
#include "llvm/LinkAllVMCore.h"
#include "llvm/PassManager.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PassNameParser.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Target/TargetData.h"


#include "MvmGC.h"
#include "mvm/Config/config.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "j3/JavaJITCompiler.h"

#include "CommandLine.h"

using namespace j3;
using namespace llvm;

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
  // Disable the lcean shutdown, as deamon threads may still
  // continue to execute and use LLVM things.
  //llvm::llvm_shutdown_obj X;
  
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
  
  mvm::MvmModule::initialise(Fast ? CodeGenOpt::None : CodeGenOpt::Aggressive);
  mvm::Collector::initialise();

  if (VMToRun == RunJava) {
#if WITH_J3
    JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
    JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(JCL);
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
#if WITH_J3
    JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
    JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
    MyCl.vmlets["java"] = (create_vm_t)(mvm::VirtualMachine::createJVM);
    MyCl.compilers["java"] = (mvm::Object*)JCL;
#endif
#if WITH_N3
    mvm::CompilationUnit* CLICompiler = 
      mvm::VirtualMachine::initialiseCLIVM();
    MyCl.vmlets["net"] = (create_vm_t)(mvm::VirtualMachine::createCLIVM);
    MyCl.compilers["net"] = (mvm::Object*)CLICompiler;
#endif
    MyCl.start();
  }

  return 0;
}
