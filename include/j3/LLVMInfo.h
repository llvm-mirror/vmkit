//===------------- LLVMInfo.h - Compiler info for LLVM --------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_LLVM_INFO_H
#define J3_LLVM_INFO_H

namespace llvm {
  class Constant;
  class Function;
  class FunctionType;
  class GCFunctionInfo;
  class MDNode;
  class Module;
  class Type;
  class Value;
}

namespace j3 {

class Class;
class JavaField;
class JavaMethod;
class Signdef;

class LLVMAssessorInfo {
public:
  const llvm::Type* llvmType;
  const llvm::Type* llvmTypePtr;
  uint8_t logSizeInBytesConstant;
};

class LLVMClassInfo : public mvm::JITInfo {
  friend class JavaAOTCompiler;
  friend class JavaJITCompiler;
  friend class JavaLLVMCompiler;
private:
  Class* classDef;
  /// virtualSizeLLVM - The LLVM constant size of instances of this class.
  ///
  llvm::Constant* virtualSizeConstant;
  /// virtualType - The LLVM type of instance of this class.
  ///
  const llvm::Type * virtualType;

  /// staticType - The LLVM type of the static instance of this class.
  ///
  const llvm::Type * staticType;
public:
  
  llvm::Value* getVirtualSize();
  const llvm::Type* getVirtualType();
  const llvm::Type* getStaticType();
  
  LLVMClassInfo(Class* cl) :
    classDef(cl),
    virtualSizeConstant(0),
    virtualType(0),
    staticType(0) {}

  virtual void clear() {
    virtualType = 0;
    staticType = 0;
    virtualSizeConstant = 0;
  }
};

class LLVMMethodInfo : public mvm::JITInfo {
private:
  JavaMethod* methodDef;

  llvm::Function* methodFunction;
  llvm::Constant* offsetConstant;
  const llvm::FunctionType* functionType;
  llvm::MDNode* DbgSubprogram;
  
  
public:
  llvm::Function* getMethod();
  llvm::Constant* getOffset();
  const llvm::FunctionType* getFunctionType();
    
  LLVMMethodInfo(JavaMethod* M) :  methodDef(M), methodFunction(0),
    offsetConstant(0), functionType(0), DbgSubprogram(0) {}
 
  void setDbgSubprogram(llvm::MDNode* node) { DbgSubprogram = node; }
  llvm::MDNode* getDbgSubprogram() { return DbgSubprogram; }

  virtual void clear() {
    methodFunction = 0;
    offsetConstant = 0;
    functionType = 0;
    DbgSubprogram = 0;
  }
};


class LLVMFieldInfo : public mvm::JITInfo {
private:
  JavaField* fieldDef;
  
  llvm::Constant* offsetConstant;

public:
  llvm::Constant* getOffset();

  LLVMFieldInfo(JavaField* F) : 
    fieldDef(F), 
    offsetConstant(0) {}

  virtual void clear() {
    offsetConstant = 0;
  }
};

class LLVMSignatureInfo : public mvm::JITInfo {
private:
  const llvm::FunctionType* staticType;
  const llvm::FunctionType* virtualType;
  const llvm::FunctionType* nativeType;
  
  const llvm::FunctionType* virtualBufType;
  const llvm::FunctionType* staticBufType;

  const llvm::PointerType* staticPtrType;
  const llvm::PointerType* virtualPtrType;
  const llvm::PointerType* nativePtrType;
  
  llvm::Function* virtualBufFunction;
  llvm::Function* virtualAPFunction;
  llvm::Function* staticBufFunction;
  llvm::Function* staticAPFunction;
  
  llvm::Function* staticStubFunction;
  llvm::Function* specialStubFunction;
  llvm::Function* virtualStubFunction;
  
  Signdef* signature;

  llvm::Function* createFunctionCallBuf(bool virt);
  llvm::Function* createFunctionCallAP(bool virt);
  llvm::Function* createFunctionStub(bool special, bool virt);
   
  
public:
  const llvm::FunctionType* getVirtualType();
  const llvm::FunctionType* getStaticType();
  const llvm::FunctionType* getNativeType();

  const llvm::FunctionType* getVirtualBufType();
  const llvm::FunctionType* getStaticBufType();

  const llvm::PointerType*  getStaticPtrType();
  const llvm::PointerType*  getNativePtrType();
  const llvm::PointerType*  getVirtualPtrType();
   
  llvm::Function* getVirtualBuf();
  llvm::Function* getVirtualAP();
  llvm::Function* getStaticBuf();
  llvm::Function* getStaticAP();
  
  llvm::Function* getStaticStub();
  llvm::Function* getSpecialStub();
  llvm::Function* getVirtualStub();
  
  LLVMSignatureInfo(Signdef* sign) : 
    staticType(0),
    virtualType(0),
    nativeType(0),
    virtualBufType(0),
    staticBufType(0),
    staticPtrType(0),
    virtualPtrType(0),
    nativePtrType(0),
    virtualBufFunction(0),
    virtualAPFunction(0),
    staticBufFunction(0),
    staticAPFunction(0),
    staticStubFunction(0),
    specialStubFunction(0),
    virtualStubFunction(0),
    signature(sign) {}

};

} // end namespace j3

#endif
