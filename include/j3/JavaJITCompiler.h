//===------- JavaJITCompiler.h - The J3 just in time compiler -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_JIT_COMPILER_H
#define J3_JIT_COMPILER_H

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "j3/JavaLLVMCompiler.h"

namespace j3 {

class JavaJITCompiler;

class JavaJITListener : public llvm::JITEventListener {
  JavaJITCompiler* TheCompiler;
public:
  JavaJITListener(JavaJITCompiler* Compiler) {
    TheCompiler = Compiler;
  }

  virtual void NotifyFunctionEmitted(
      const llvm::Function &F,
      void *Code,
      size_t Size,
      const llvm::JITEventListener::EmittedFunctionDetails &Details);
};

class JavaJITCompiler : public JavaLLVMCompiler {
public:

  bool EmitFunctionName;
  JavaJITListener listener;
  llvm::ExecutionEngine* executionEngine;
  llvm::GCStrategy* TheGCStrategy;

  JavaJITCompiler(const std::string &ModuleID, bool trusted = false);
  ~JavaJITCompiler();
  
  virtual bool isStaticCompiling() {
    return false;
  }

  virtual void* GenerateStub(llvm::Function* F);  
 
  virtual bool emitFunctionName() {
    return EmitFunctionName;
  }

  virtual void makeVT(Class* cl);
  virtual void makeIMT(Class* cl);
  
  virtual void* materializeFunction(JavaMethod* meth);
  
  virtual llvm::Constant* getFinalObject(JavaObject* obj, CommonClass* cl);
  virtual JavaObject* getFinalObject(llvm::Value* C);
  virtual llvm::Constant* getNativeClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl);
  virtual llvm::Constant* getStaticInstance(Class* cl);
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*);
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth);
  
  virtual llvm::Constant* getString(JavaString* str);
  virtual llvm::Constant* getStringPtr(JavaString** str);
  virtual llvm::Constant* getConstantPool(JavaConstantPool* ctp);
  virtual llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr);
  
  virtual void setMethod(llvm::Function* func, void* ptr, const char* name);
  

#ifdef SERVICE
  virtual llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where);
#endif
 
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert) = 0;
  virtual uintptr_t getPointerOrStub(JavaMethod& meth, int type) = 0;

#ifdef WITH_LLVM_GCC
  virtual mvm::StackScanner* createStackScanner() {
    if (useCooperativeGC())
      return new mvm::PreciseStackScanner();
    
    return new mvm::UnpreciseStackScanner();
  }
#endif
  
  static JavaJITCompiler* CreateCompiler(const std::string& ModuleID);

};

class JavaJ3LazyJITCompiler : public JavaJITCompiler {
public:
  virtual bool needsCallback(JavaMethod* meth, bool* needsInit);
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert);
  virtual uintptr_t getPointerOrStub(JavaMethod& meth, int side);
  
  virtual JavaCompiler* Create(const std::string& ModuleID) {
    return new JavaJ3LazyJITCompiler(ModuleID, false);
  }

  JavaJ3LazyJITCompiler(const std::string& ModuleID, bool trusted);
};

} // end namespace j3

#endif
