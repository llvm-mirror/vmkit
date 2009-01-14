//===----------------- vmjc.cpp - Java static compiler --------------------===//
//
//                           The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This utility may be invoked in the following manner:
//  vmjc [options] x - Read Java bytecode from the x.class file, write llvm
//                     bytecode to the x.bc file.
//  Options:
//      --help   - Output information about command line switches
//
//===----------------------------------------------------------------------===//

#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Config/config.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PassNameParser.h"
#include "llvm/Support/Streams.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/System/Signals.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachineRegistry.h"
#include "llvm/Target/TargetMachine.h"

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

using namespace llvm;

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input Java bytecode>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Override output filename"),
               cl::value_desc("filename"));

static cl::opt<bool>
Force("f", cl::desc("Overwrite output files"));

static cl::opt<bool>
DontPrint("disable-output", cl::desc("Don't output the .ll file"), cl::Hidden);


// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static llvm::cl::list<const llvm::PassInfo*, bool, llvm::PassNameParser>
PassList(llvm::cl::desc("Optimizations available:"));


static cl::opt<bool> 
DisableOptimizations("disable-opt", 
                     cl::desc("Do not run any optimization passes"));

static cl::opt<bool>
StandardCompileOpts("std-compile-opts", 
                   cl::desc("Include the standard compile time optimizations"));

static cl::opt<std::string>
TargetTriple("mtriple", cl::desc("Override target triple for module"));


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


int main(int argc, char **argv) {
  llvm_shutdown_obj X;  // Call llvm_shutdown() on exit.
  try {
    cl::ParseCommandLineOptions(argc, argv, "vmkit .class -> .ll compiler\n");
    sys::PrintStackTraceOnErrorSignal();

    std::ostream *Out = &std::cout;  // Default to printing to stdout.
    std::string ErrorMessage;

    
    if (InputFilename == "-") {
      cl::PrintHelpMessage();
      return 0;
    }
    
    Module* TheModule = new Module("bootstrap module");
    if (!TargetTriple.empty())
      TheModule->setTargetTriple(TargetTriple);
    else
      TheModule->setTargetTriple(LLVM_HOSTTRIPLE);
    
    // Create the TargetMachine we will be generating code with.
    std::string Err; 
    const TargetMachineRegistry::entry *TME = 
      TargetMachineRegistry::getClosestStaticTargetForModule(*TheModule, Err);
    if (!TME) {
      cerr << "Did not get a target machine!\n";
      exit(1);
    }

    std::string FeatureStr;
    TargetMachine* TheTarget = TME->CtorFn(*TheModule, FeatureStr);

    // Install information about target datalayout stuff into the module for
    // optimizer use.
    TheModule->setDataLayout(TheTarget->getTargetData()->
                             getStringRepresentation());


    mvm::MvmModule::initialise(false, TheModule, TheTarget);
    mvm::Object::initialise();
    Collector::initialise(0);
    Collector::enable(0);

    mvm::CompilationUnit* CU = mvm::VirtualMachine::initialiseJVM(true);
    addCommandLinePass(CU, argv); 
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(CU);
    vm->compile(InputFilename.c_str());
    vm->waitForExit();

    if (DontPrint) {
      // Just use stdout.  We won't actually print anything on it.
    } else if (OutputFilename != "") {   // Specified an output filename?
      if (OutputFilename != "-") { // Not stdout?
        if (!Force && std::ifstream(OutputFilename.c_str())) {
          // If force is not specified, make sure not to overwrite a file!
          cerr << argv[0] << ": error opening '" << OutputFilename
               << "': file exists! Sending to standard output.\n";
        } else {
          Out = new std::ofstream(OutputFilename.c_str());
        }
      }
    } else {
      if (InputFilename == "-") {
        OutputFilename = "-";
      } else {
        std::string IFN = InputFilename;
        int Len = IFN.length();
        if (IFN[Len-3] == '.' && IFN[Len-2] == 'b' && IFN[Len-1] == 'c') {
          // Source ends in .bc
          OutputFilename = std::string(IFN.begin(), IFN.end()-3)+".bc";
        } else {
          OutputFilename = IFN+".bc";
        }

        if (!Force && std::ifstream(OutputFilename.c_str())) {
          // If force is not specified, make sure not to overwrite a file!
          cerr << argv[0] << ": error opening '" << OutputFilename
               << "': file exists! Sending to standard output.\n";
        } else {
          Out = new std::ofstream(OutputFilename.c_str());

          // Make sure that the Out file gets unlinked from the disk if we get a
          // SIGINT
          sys::RemoveFileOnSignal(sys::Path(OutputFilename));
        }
      }
    }

    if (!Out->good()) {
      cerr << argv[0] << ": error opening " << OutputFilename
           << ": sending to stdout instead!\n";
      Out = &std::cout;
    }
    
    if (Force || !CheckBitcodeOutputToConsole(Out,true))
      WriteBitcodeToFile(CU->TheModule, *Out);

    if (Out != &std::cout) {
      ((std::ofstream*)Out)->close();
      delete Out;
    }
    return 0;
  } catch (const std::string& msg) {
    cerr << argv[0] << ": " << msg << "\n";
  } catch (...) {
    cerr << argv[0] << ": Unexpected unknown exception occurred.\n";
  }
  return 1;
}

