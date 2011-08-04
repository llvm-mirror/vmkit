//===------------------------ Precompiler.cpp -----------------------------===//
// Run a program and emit code for classes loaded by the bootstrap loader -===//
//
//                           The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Path.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/Signals.h"

#include "MvmGC.h"
#include "mvm/JIT.h"
#include "mvm/MethodInfo.h"
#include "mvm/Object.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include "j3/JavaAOTCompiler.h"
#include "j3/JavaJITCompiler.h"
#include "../../lib/J3/VMCore/JavaThread.h"
#include "../../lib/J3/VMCore/JnjvmClassLoader.h"
#include "../../lib/J3/VMCore/Jnjvm.h"

#include <string>

extern llvm::cl::opt<bool> StandardCompileOpts;

using namespace j3;
using namespace mvm;

#include "FrametablesExterns.inc"

CompiledFrames* frametables[] = {
  #include "FrametablesSymbols.inc"
  NULL
};


static void mainCompilerLoaderStart(JavaThread* th) {
  Jnjvm* vm = th->getJVM();
  JnjvmBootstrapLoader* bootstrapLoader = vm->bootstrapLoader;
  JavaAOTCompiler* M = (JavaAOTCompiler*)bootstrapLoader->getCompiler();
  M->compileClassLoader(bootstrapLoader);
  vm->exit(); 
}


int main(int argc, char **argv, char **envp) {
  llvm::llvm_shutdown_obj X;

  // Initialize base components.  
  MvmModule::initialise();
  Collector::initialise();
  
  // Tell the compiler to run all optimizations.
  StandardCompileOpts = true;
 
  // Create the allocator that will allocate the bootstrap loader and the JVM.
  mvm::BumpPtrAllocator Allocator;
  JavaJITCompiler* JIT = JavaJITCompiler::CreateCompiler("JIT");
  JnjvmBootstrapLoader* loader = new(Allocator, "Bootstrap loader")
    JnjvmBootstrapLoader(Allocator, JIT, true);
  Jnjvm* vm = new(Allocator, "VM") Jnjvm(Allocator, frametables, loader);
 
  // Run the application. 
  vm->runApplication(argc, argv);
  vm->waitForExit();

  // Now AOT Compile all compiled methods.
  JavaAOTCompiler* AOT = new JavaAOTCompiler("AOT");
  loader->setCompiler(AOT);

  vm->doExit = false;
  JavaThread* th = new JavaThread(vm);
  vm->setMainThread(th);
  th->start((void (*)(mvm::Thread*))mainCompilerLoaderStart);
  vm->waitForExit();

  AOT->printStats();

  // Emit the bytecode in file.
  std::string OutputFilename = "generated.bc";
  std::string ErrorInfo;
  std::auto_ptr<llvm::raw_ostream> Out 
    (new llvm::raw_fd_ostream(OutputFilename.c_str(), ErrorInfo,
                        llvm::raw_fd_ostream::F_Binary));
  if (!ErrorInfo.empty()) {
    llvm::errs() << ErrorInfo << '\n';
    return 1;
  }
   
  // Make sure that the Out file gets unlinked from the disk if we get a
  // SIGINT.
  llvm::sys::RemoveFileOnSignal(llvm::sys::Path(OutputFilename));
  
  llvm::WriteBitcodeToFile(AOT->getLLVMModule(), *Out);

  return 0;
}
