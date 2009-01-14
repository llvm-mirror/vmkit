//===--------- Launcher.cpp - Launch command line -------------------------===//
//
//                            JnJVM
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


#include "MvmGC.h"
#include "mvm/Config/config.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "CommandLine.h"

using namespace llvm;

enum VMType {
  Interactive, RunJava, RunNet
};

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static llvm::cl::list<const llvm::PassInfo*, bool, llvm::PassNameParser>
PassList(llvm::cl::desc("Optimizations available:"));



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


static cl::opt<bool> 
DisableOptimizations("disable-opt", 
                     cl::desc("Do not run any optimization passes"));

static cl::opt<bool>
StandardCompileOpts("std-compile-opts", 
                   cl::desc("Include the standard compile time optimizations"));

inline void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}


void addCommandLinePass(mvm::CompilationUnit* CU, char** argv) {
  FunctionPassManager* Passes = CU->FunctionPasses;

  // Create a new optimization pass for each one specified on the command line
  for (unsigned i = 0; i < PassList.size(); ++i) {
    // Check to see if -std-compile-opts was specified before this option.  If
    // so, handle it.
    if (StandardCompileOpts && 
        StandardCompileOpts.getPosition() < PassList.getPosition(i)) {
      if (!DisableOptimizations) CU->AddStandardCompilePasses();
      StandardCompileOpts = false;
    }
      
    const PassInfo *PassInf = PassList[i];
    Pass *P = 0;
    if (PassInf->getNormalCtor())
      P = PassInf->getNormalCtor()();
    else
      cerr << argv[0] << ": cannot create pass: "
           << PassInf->getPassName() << "\n";
    if (P) {
        bool isModulePass = dynamic_cast<ModulePass*>(P) != 0;
        if (isModulePass) 
          cerr << argv[0] << ": vmkit does not support module pass: "
             << PassInf->getPassName() << "\n";
        else addPass(Passes, P);

    }
  }
    
  // If -std-compile-opts was specified at the end of the pass list, add them.
  if (StandardCompileOpts) {
    CU->AddStandardCompilePasses();
  }    

}

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
    addCommandLinePass(CU, argv);
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
    addCommandLinePass(JVMCompiler, argv);
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
