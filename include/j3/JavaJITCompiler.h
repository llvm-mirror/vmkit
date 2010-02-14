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

#include "j3/JavaLLVMCompiler.h"

namespace j3 {

class JavaJITCompiler : public JavaLLVMCompiler {
public:

  bool EmitFunctionName;

  JavaJITCompiler(const std::string &ModuleID);
  
  virtual bool isStaticCompiling() {
    return false;
  }
 
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
  
  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name);
  

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
    return new JavaJ3LazyJITCompiler(ModuleID);
  }

  JavaJ3LazyJITCompiler(const std::string& ModuleID);
};

} // end namespace j3

#endif
