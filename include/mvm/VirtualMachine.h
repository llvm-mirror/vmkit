//===--------- VirtualMachine.h - Registering a VM ------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Ultimately, this would be like a generic way of defining a VM. But we're not
// quite there yet.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_VIRTUALMACHINE_H
#define MVM_VIRTUALMACHINE_H

#include "mvm/Allocator.h"
#include "mvm/CompilationUnit.h"
#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"

namespace mvm {

/// VirtualMachine - This class is the root of virtual machine classes. It
/// defines what a VM should be.
///
class VirtualMachine : public mvm::Object {
public:
  
  /// runApplication - Run an application. The application name is in
  /// the arguments, hence it is the virtual machine's job to parse them.
  virtual void runApplication(int argc, char** argv) = 0;
  
  /// compile - Compile a given file to LLVM.
  virtual void compile(const char* name) = 0;
 
  /// waitForExit - Wait until the virtual machine stops its execution.
  virtual void waitForExit() = 0;

  static CompilationUnit* initialiseJVM(bool staticCompilation = false);
  static VirtualMachine* createJVM(CompilationUnit* C = 0);
  
  static CompilationUnit* initialiseCLIVM();
  static VirtualMachine* createCLIVM(CompilationUnit* C = 0);
  

#ifdef ISOLATE
  size_t IsolateID;
#endif

#ifdef SERVICE
  size_t status;
  uint64_t memoryUsed;
  uint64_t gcTriggered;
  uint64_t executionTime;
  uint64_t numThreads;
  CompilationUnit* CU;
  virtual void stopService() {}

  uint64_t memoryLimit;
  uint64_t executionLimit;
#endif

  mvm::Allocator gcAllocator;

};


} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
