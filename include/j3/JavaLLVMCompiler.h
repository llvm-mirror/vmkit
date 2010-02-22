//===---==---- JavaLLVMCompiler.h - A LLVM Compiler for J3 ----------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_LLVM_COMPILER_H
#define J3_LLVM_COMPILER_H

#include "j3/JavaCompiler.h"
#include "j3/J3Intrinsics.h"
#include "j3/LLVMInfo.h"

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

namespace llvm {
  class BasicBlock;
  class DIFactory;
}

namespace j3 {

class CommonClass;
class Class;
class ClassArray;
class ClassPrimitive;
class JavaConstantPool;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaString;
class JavaVirtualTable;
class Jnjvm;
class Typedef;
class Signdef;

class JavaLLVMCompiler : public JavaCompiler {
  friend class LLVMClassInfo;
  friend class LLVMMethodInfo;

protected:
  llvm::Module* TheModule;
  llvm::DIFactory* DebugFactory;  
  J3Intrinsics JavaIntrinsics;

  void addJavaPasses();

private:  
  bool enabledException;
  bool cooperativeGC;
  
  virtual void makeVT(Class* cl) = 0;
  virtual void makeIMT(Class* cl) = 0;
  
  std::map<llvm::Function*, JavaMethod*> functions;  
  typedef std::map<llvm::Function*, JavaMethod*>::iterator function_iterator;

  std::map<llvm::MDNode*, JavaMethod*> DbgInfos;
  typedef std::map<llvm::MDNode*, JavaMethod*>::iterator dbg_iterator;

public:
  JavaLLVMCompiler(const std::string &ModuleID);
  
  virtual bool isStaticCompiling() = 0;
  virtual bool emitFunctionName() = 0;
  
  llvm::DIFactory* getDebugFactory() {
    return DebugFactory;
  }

  llvm::Module* getLLVMModule() {
    return TheModule;
  }
  
  llvm::LLVMContext& getLLVMContext() {
    return TheModule->getContext();
  }

  J3Intrinsics* getIntrinsics() {
    return &JavaIntrinsics;
  }

  bool hasExceptionsEnabled() {
    return enabledException;
  }

  bool useCooperativeGC() {
    return cooperativeGC;
  }
  
  void disableExceptions() {
    enabledException = false;
  }
  
  void disableCooperativeGC() {
    cooperativeGC = false;
  }
 
  virtual JavaCompiler* Create(const std::string& ModuleID) = 0;
  
  virtual ~JavaLLVMCompiler();

  JavaMethod* getJavaMethod(llvm::Function*);
  llvm::MDNode* GetDbgSubprogram(JavaMethod* meth);

  void resolveVirtualClass(Class* cl);
  void resolveStaticClass(Class* cl);
  static llvm::Function* getMethod(JavaMethod* meth);

  void initialiseAssessorInfo();
  std::map<const char, LLVMAssessorInfo> AssessorInfo;
  LLVMAssessorInfo& getTypedefInfo(const Typedef* type);

  static LLVMSignatureInfo* getSignatureInfo(Signdef* sign);
  static LLVMClassInfo* getClassInfo(Class* cl);
  static LLVMFieldInfo* getFieldInfo(JavaField* field);
  static LLVMMethodInfo* getMethodInfo(JavaMethod* method);
  
  virtual llvm::Constant* getFinalObject(JavaObject* obj, CommonClass* cl) = 0;
  virtual JavaObject* getFinalObject(llvm::Value* C) = 0;
  virtual llvm::Constant* getNativeClass(CommonClass* cl) = 0;
  virtual llvm::Constant* getJavaClass(CommonClass* cl) = 0;
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl) = 0;
  virtual llvm::Constant* getStaticInstance(Class* cl) = 0;
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*) = 0;
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth) = 0;
  
  virtual llvm::Constant* getString(JavaString* str) = 0;
  virtual llvm::Constant* getStringPtr(JavaString** str) = 0;
  virtual llvm::Constant* getConstantPool(JavaConstantPool* ctp) = 0;
  virtual llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr) = 0;
  
  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name) = 0;
  
#ifdef SERVICE
  virtual llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where) = 0;
#endif
  
  virtual void* materializeFunction(JavaMethod* meth) = 0;
  llvm::Function* parseFunction(JavaMethod* meth);
   
  llvm::FunctionPassManager* JavaFunctionPasses;
  llvm::FunctionPassManager* JavaNativeFunctionPasses;
  
  virtual bool needsCallback(JavaMethod* meth, bool* needsInit) {
    *needsInit = true;
    return meth == NULL;
  }
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert) = 0;
  
  virtual void staticCallBuf(Signdef* sign) {
    getSignatureInfo(sign)->getStaticBuf();
  }

  virtual void virtualCallBuf(Signdef* sign) {
    getSignatureInfo(sign)->getVirtualBuf();
  }

  virtual void staticCallAP(Signdef* sign) {
    getSignatureInfo(sign)->getStaticAP();
  }

  virtual void virtualCallAP(Signdef* sign) {
    getSignatureInfo(sign)->getVirtualAP();
  }
  
  virtual void virtualCallStub(Signdef* sign) {
    getSignatureInfo(sign)->getVirtualStub();
  }
  
  virtual void specialCallStub(Signdef* sign) {
    getSignatureInfo(sign)->getSpecialStub();
  }
  
  virtual void staticCallStub(Signdef* sign) {
    getSignatureInfo(sign)->getStaticStub();
  }

  llvm::Function* NativeLoader;
};

} // end namespace j3

#endif
