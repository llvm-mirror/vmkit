//===------- MvmMemoryManager.h - LLVM Memory manager for Mvm -------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_MEMORY_MANAGER_H
#define MVM_MEMORY_MANAGER_H

#include <map>

#include <llvm/ExecutionEngine/JITMemoryManager.h>

using namespace llvm;

namespace mvm {

class Code;

/// MvmMemoryManager - This class is a wrapper to the default JITMemoryManager
/// in LLVM. It creates Code objects for backtraces and getting virtual machine
/// information out of dynamically generated native code.
///
class MvmMemoryManager : public JITMemoryManager {
  
  /// currentMethod -  Current method being compiled.
  ///
  Code* currentMethod;
  
  /// realMemoryManager - The real allocator 
  JITMemoryManager* realMemoryManager;

public:
  
  MvmMemoryManager() : JITMemoryManager() { 
    realMemoryManager = JITMemoryManager::CreateDefaultMemManager();
  }
  
  /// startFunctionBody - When we start JITing a function, the JIT calls this 
  /// method to allocate a block of free RWX memory, which returns a pointer to
  /// it.  The JIT doesn't know ahead of time how much space it will need to
  /// emit the function, so it doesn't pass in the size.  Instead, this method
  /// is required to pass back a "valid size".  The JIT will be careful to not
  /// write more than the returned ActualSize bytes of memory. 
  virtual unsigned char *startFunctionBody(const Function *F, 
                                           uintptr_t &ActualSize);
  
  /// allocateStub - This method is called by the JIT to allocate space for a
  /// function stub (used to handle limited branch displacements) while it is
  /// JIT compiling a function.  For example, if foo calls bar, and if bar
  /// either needs to be lazily compiled or is a native function that exists too
  /// far away from the call site to work, this method will be used to make a
  /// thunk for it.  The stub should be "close" to the current function body,
  /// but should not be included in the 'actualsize' returned by
  /// startFunctionBody.
  virtual unsigned char *allocateStub(const GlobalValue* GV, unsigned StubSize,
                                      unsigned Alignment);
  
  /// endFunctionBody - This method is called when the JIT is done codegen'ing
  /// the specified function.  At this point we know the size of the JIT
  /// compiled function.  This passes in FunctionStart (which was returned by
  /// the startFunctionBody method) and FunctionEnd which is a pointer to the 
  /// actual end of the function.  This method should mark the space allocated
  /// and remember where it is in case the client wants to deallocate it.
  virtual void endFunctionBody(const Function *F, unsigned char *FunctionStart,
                               unsigned char *FunctionEnd);
  
  /// deallocateMemForFunction - Free JIT memory for the specified function.
  /// This is never called when the JIT is currently emitting a function.
  virtual void deallocateMemForFunction(const Function *F);
  
  /// AllocateGOT - If the current table requires a Global Offset Table, this
  /// method is invoked to allocate it.  This method is required to set HasGOT
  /// to true.
  virtual void AllocateGOT();

  /// getGOTBase - If this is managing a Global Offset Table, this method should
  /// return a pointer to its base.
  virtual unsigned char *getGOTBase() const;
  

  /// startExceptionTable - When we finished JITing the function, if exception
  /// handling is set, we emit the exception table.
  virtual unsigned char* startExceptionTable(const Function* F,
                                             uintptr_t &ActualSize);
  
  /// endExceptionTable - This method is called when the JIT is done emitting
  /// the exception table.
  virtual void endExceptionTable(const Function *F, unsigned char *TableStart,
                               unsigned char *TableEnd, 
                               unsigned char* FrameRegister);
};

} // End mvm namespace

#endif
