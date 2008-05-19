//===------- JnjvmModule.h - Definition of a Jnjvm module -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <map>

#include "llvm/Constant.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Value.h"

namespace jnjvm {

class CommonClass;
class Class;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaJIT;
class JnjvmModule;
class Signdef;

class LLVMCommonClassInfo {
  
  friend class JnjvmModule;

protected:
  CommonClass* classDef;
  JnjvmModule* module;

private:
  /// varGV - The LLVM global variable representing this class.
  ///
  llvm::GlobalVariable* varGV;

#ifndef MULTIPLE_VM
  /// delegateeGV - The LLVM global variable representing the 
  /// java/lang/Class instance of this class.
  llvm::GlobalVariable* delegateeGV;
#endif


public:
  llvm::Value* getVar(JavaJIT* jit);
  llvm::Value* getDelegatee(JavaJIT* jit);
  
  LLVMCommonClassInfo(CommonClass* cl, JnjvmModule* M) : 
    classDef(cl),
    module(M),
    varGV(0),
    delegateeGV(0) {}
};

class LLVMClassInfo : public LLVMCommonClassInfo {
  friend class JnjvmModule;
private:
  /// virtualSizeLLVM - The LLVM constant size of instances of this class.
  ///
  llvm::ConstantInt* virtualSizeConstant;
  llvm::GlobalVariable* staticVarGV;
  llvm::GlobalVariable* virtualTableGV;
  llvm::Function* virtualTracerFunction;
  llvm::Function* staticTracerFunction;
  /// virtualType - The LLVM type of instance of this class.
  ///
  const llvm::Type * virtualType;

  /// staticType - The LLVM type of the static instance of this class.
  ///
  const llvm::Type * staticType;
public:
  
  llvm::Value* getStaticVar(JavaJIT* jit);
  llvm::Value* getVirtualTable(JavaJIT* jit);
  llvm::Value* getVirtualSize(JavaJIT* jit);
  llvm::Function* getStaticTracer();
  llvm::Function* getVirtualTracer();
  const llvm::Type* getVirtualType();
  const llvm::Type* getStaticType();
  
  LLVMClassInfo(Class* cl, JnjvmModule* M) : 
    LLVMCommonClassInfo((CommonClass*)cl, M),
    virtualSizeConstant(0),
    staticVarGV(0),
    virtualTableGV(0),
    virtualTracerFunction(0),
    staticTracerFunction(0),
    virtualType(0),
    staticType(0) {}
};

class LLVMMethodInfo {
private:
  JavaMethod* methodDef;
  JnjvmModule* module;

  llvm::Function* methodFunction;
  llvm::ConstantInt* offsetConstant;
  const llvm::FunctionType* functionType;
  
public:
  llvm::Function* getMethod();
  llvm::ConstantInt* getOffset();
  const llvm::FunctionType* getFunctionType();
  
  LLVMMethodInfo(JavaMethod* M, JnjvmModule* Mo) : 
    methodDef(M), 
    module(Mo),
    methodFunction(0),
    offsetConstant(0),
    functionType(0) {}
};

class LLVMFieldInfo {
private:
  JavaField* fieldDef;
  JnjvmModule* module;
  
  llvm::ConstantInt* offsetConstant;

public:
  llvm::ConstantInt* getOffset();

  LLVMFieldInfo(JavaField* F, JnjvmModule* M) : 
    fieldDef(F), 
    module(M),
    offsetConstant(0) {}
};

class LLVMSignatureInfo {
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
  
  Signdef* signature;

  llvm::Function* createFunctionCallBuf(bool virt);
  llvm::Function* createFunctionCallAP(bool virt);
   
  

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
    signature(sign) {}

};

class JnjvmModule : public llvm::Module {
  friend class LLVMClassInfo;
private:
  std::map<const CommonClass*, LLVMCommonClassInfo*> classMap;
  std::map<const Signdef*, LLVMSignatureInfo*> signatureMap;
  std::map<const JavaField*, LLVMFieldInfo*> fieldMap;
  std::map<const JavaMethod*, LLVMMethodInfo*> methodMap;
  
  typedef std::map<const CommonClass*, LLVMCommonClassInfo*>::iterator
    class_iterator;  
  
  typedef std::map<const Signdef*, LLVMSignatureInfo*>::iterator
    signature_iterator;  
  
  typedef std::map<const JavaMethod*, LLVMMethodInfo*>::iterator
    method_iterator;  
  
  typedef std::map<const JavaField*, LLVMFieldInfo*>::iterator
    field_iterator;  
  
  VirtualTable* makeVT(Class* cl, bool stat);
  VirtualTable* allocateVT(Class* cl, std::vector<JavaMethod*>::iterator meths);


public:
  static llvm::ConstantInt* JavaArraySizeOffsetConstant;
  static llvm::ConstantInt* JavaArrayElementsOffsetConstant;
  static llvm::ConstantInt* JavaObjectLockOffsetConstant;
  static llvm::ConstantInt* JavaObjectClassOffsetConstant;

  static const llvm::Type* JavaArrayUInt8Type;
  static const llvm::Type* JavaArraySInt8Type;
  static const llvm::Type* JavaArrayUInt16Type;
  static const llvm::Type* JavaArraySInt16Type;
  static const llvm::Type* JavaArrayUInt32Type;
  static const llvm::Type* JavaArraySInt32Type;
  static const llvm::Type* JavaArrayLongType;
  static const llvm::Type* JavaArrayFloatType;
  static const llvm::Type* JavaArrayDoubleType;
  static const llvm::Type* JavaArrayObjectType;
  
  static const llvm::Type* VTType;
  static const llvm::Type* JavaObjectType;
  static const llvm::Type* JavaArrayType;
  static const llvm::Type* JavaClassType;
  static const llvm::Type* JavaCacheType;
  static const llvm::Type* EnveloppeType;
  static const llvm::Type* CacheNodeType;
  
#ifdef WITH_TRACER
  static llvm::Function* MarkAndTraceFunction;
  static const llvm::FunctionType* MarkAndTraceType;
  static llvm::Function* JavaObjectTracerFunction;  
#endif
  
  static llvm::Function* GetSJLJBufferFunction;
  static llvm::Function* InterfaceLookupFunction;
  static llvm::Function* FieldLookupFunction;
  static llvm::Function* PrintExecutionFunction;
  static llvm::Function* PrintMethodStartFunction;
  static llvm::Function* PrintMethodEndFunction;
  static llvm::Function* JniProceedPendingExceptionFunction;
  static llvm::Function* InitialisationCheckFunction;
  static llvm::Function* ForceInitialisationCheckFunction;
  static llvm::Function* ClassLookupFunction;
#ifndef WITHOUT_VTABLE
  static llvm::Function* VirtualLookupFunction;
#endif
  static llvm::Function* InstanceOfFunction;
  static llvm::Function* AquireObjectFunction;
  static llvm::Function* ReleaseObjectFunction;
#ifdef SERVICE_VM
  static llvm::Function* AquireObjectInSharedDomainFunction;
  static llvm::Function* ReleaseObjectInSharedDomainFunction;
  static llvm::Function* ServiceCallStartFunction;
  static llvm::Function* ServiceCallStopFunction;
#endif
  static llvm::Function* MultiCallNewFunction;
  static llvm::Function* RuntimeUTF8ToStrFunction;
  static llvm::Function* GetStaticInstanceFunction;
  static llvm::Function* GetClassDelegateeFunction;
  static llvm::Function* ArrayLengthFunction;
  static llvm::Function* GetVTFunction;
  static llvm::Function* GetClassFunction;
  static llvm::Function* JavaObjectAllocateFunction;
#ifdef MULTIPLE_GC
  static llvm::Function* GetCollectorFunction;
#endif
  static llvm::Function* GetVTFromClassFunction;
  static llvm::Function* GetObjectSizeFromClassFunction;
  static llvm::ConstantInt* OffsetObjectSizeInClassConstant;
  static llvm::ConstantInt* OffsetVTInClassConstant;

  static llvm::Constant* JavaClassNullConstant;

  static llvm::Constant*    JavaObjectNullConstant;
  static llvm::Constant*    UTF8NullConstant;
  static llvm::Constant*    MaxArraySizeConstant;
  static llvm::Constant*    JavaObjectSizeConstant;

  static llvm::GlobalVariable* ArrayObjectVirtualTableGV;
  static llvm::GlobalVariable* JavaObjectVirtualTableGV;
  
  static llvm::Function* GetExceptionFunction;
  static llvm::Function* GetJavaExceptionFunction;
  static llvm::Function* ThrowExceptionFunction;
  static llvm::Function* ClearExceptionFunction;
  static llvm::Function* CompareExceptionFunction;
  static llvm::Function* NullPointerExceptionFunction;
  static llvm::Function* IndexOutOfBoundsExceptionFunction;
  static llvm::Function* ClassCastExceptionFunction;
  static llvm::Function* OutOfMemoryErrorFunction;
  static llvm::Function* NegativeArraySizeExceptionFunction;

  static void InitField(JavaField* field);
  static void InitField(JavaField* field, JavaObject* obj, uint64 val = 0);
  static void InitField(JavaField* field, JavaObject* obj, JavaObject* val);
  static void InitField(JavaField* field, JavaObject* obj, double val);
  static void InitField(JavaField* field, JavaObject* obj, float val);


  void resolveVirtualClass(Class* cl);
  void resolveStaticClass(Class* cl);
  void setMethod(JavaMethod* meth, const char* name);

  LLVMSignatureInfo* getSignatureInfo(Signdef*);
  LLVMCommonClassInfo* getClassInfo(CommonClass*);
  LLVMFieldInfo* getFieldInfo(JavaField*);
  LLVMMethodInfo* getMethodInfo(JavaMethod*);

  explicit JnjvmModule(const std::string &ModuleID) : llvm::Module(ModuleID) {}
  void initialise();
};

}
