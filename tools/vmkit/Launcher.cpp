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
#include "../../lib/J3/VMCore/JnjvmClassLoader.h"
#include "../../lib/J3/VMCore/Jnjvm.h"

#include "CommandLine.h"

using namespace j3;
using namespace llvm;

enum VMType {
  RunJava
};

static llvm::cl::opt<VMType> VMToRun(llvm::cl::desc("Choose VM to run:"),
  llvm::cl::values(
    clEnumValN(RunJava , "java", "Run the JVM"),
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
    fprintf(stderr, "Only -java is supported\n");
    return 0;
  }
  
  mvm::MvmModule::initialise(Fast ? CodeGenOpt::None : CodeGenOpt::Aggressive);
  mvm::Collector::initialise();

  if (VMToRun == RunJava) {
    mvm::BumpPtrAllocator Allocator;
    JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
    JnjvmBootstrapLoader* loader = new(Allocator, "Bootstrap loader")
        JnjvmBootstrapLoader(Allocator, Comp, true);
    Jnjvm* vm = new(Allocator, "VM") Jnjvm(Allocator, loader);
    vm->runApplication(argc, argv);
    vm->waitForExit();
  }

  return 0;
}
