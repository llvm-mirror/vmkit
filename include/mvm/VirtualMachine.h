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

#include <cassert>
#include <map>

namespace mvm {

/// VirtualMachine - This class is the root of virtual machine classes. It
/// defines what a VM should be.
///
class VirtualMachine : public mvm::Object {
protected:

  VirtualMachine() {
#ifdef SERVICE
    memoryLimit = ~0;
    executionLimit = ~0;
    GCLimit = ~0;
    threadLimit = ~0;
    parent = this;
    status = 1;
    _since_last_collection = 4*1024*1024;
#endif
  }
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
 
protected:

  /// JavaFunctionMap - Map of Java method to function pointers. This map is
  /// used when walking the stack so that VMKit knows which Java method is
  /// executing on the stack.
  ///
  std::map<void*, void*> Functions;

  /// FunctionMapLock - Spin lock to protect the JavaFunctionMap.
  ///
  mvm::SpinLock FunctionMapLock;

public:
  /// addMethodInFunctionMap - A new method pointer in the function map.
  ///
  template <typename T>
  void addMethodInFunctionMap(T* meth, void* addr) {
    FunctionMapLock.acquire();
    Functions.insert(std::make_pair((void*)addr, meth));
    FunctionMapLock.release();
  }
  
  /// IPToJavaMethod - Map an instruction pointer to the Java method.
  ///
  template <typename T> T* IPToMethod(void* ip) {
    FunctionMapLock.acquire();
    std::map<void*, void*>::iterator I = Functions.upper_bound(ip);
    assert(I != Functions.begin() && "Wrong value in function map");
    FunctionMapLock.release();

    // Decrement because we had the "greater than" value.
    I--;
    return (T*)I->second;
  }

#ifdef ISOLATE
  size_t IsolateID;
#endif

#ifdef SERVICE
  uint64_t memoryUsed;
  uint64_t gcTriggered;
  uint64_t executionTime;
  uint64_t numThreads;
  CompilationUnit* CU;
  virtual void stopService() {}

  uint64_t memoryLimit;
  uint64_t executionLimit;
  uint64_t threadLimit;
  uint64_t GCLimit;

  int _since_last_collection;
  VirtualMachine* parent;
  uint32 status; 
#endif

  mvm::Allocator gcAllocator;

};


} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
