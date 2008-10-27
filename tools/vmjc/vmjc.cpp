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
//  vmjc [options] x.bc - Read Java bytecode from the x.class file, write llvm
//                            asm to the x.ll file.
//  Options:
//      --help   - Output information about command line switches
//
//===----------------------------------------------------------------------===//

#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Streams.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/System/Signals.h"

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include <iostream>
#include <fstream>
#include <memory>

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

int main(int argc, char **argv) {
  llvm_shutdown_obj X;  // Call llvm_shutdown() on exit.
  int base;
  try {
    cl::ParseCommandLineOptions(argc, argv, "vmkit .class -> .ll compiler\n");
    sys::PrintStackTraceOnErrorSignal();

    std::ostream *Out = &std::cout;  // Default to printing to stdout.
    std::string ErrorMessage;

    
    if (InputFilename == "-") {
      cl::PrintHelpMessage();
      return 0;
    }

    mvm::MvmModule::initialise();
    mvm::Object::initialise();
    mvm::Thread::initialise();
    Collector::initialise(0, &base);
  
    mvm::CompilationUnit* CU = mvm::VirtualMachine::initialiseJVM(true);
    mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(CU);
    vm->compile(InputFilename.c_str());


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
          OutputFilename = std::string(IFN.begin(), IFN.end()-3)+".ll";
        } else {
          OutputFilename = IFN+".ll";
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

    // All that llvm-dis does is write the assembly to a file.
    if (!DontPrint) {
      PassManager Passes;
      raw_os_ostream L(*Out);
      Passes.add(createPrintModulePass(&L));
      Passes.run(*CU->TheModule);
    }

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

