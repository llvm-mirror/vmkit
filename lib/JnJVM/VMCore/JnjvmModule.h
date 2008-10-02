//===------- JnjvmModule.h - Definition of a Jnjvm module -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_MODULE_H
#define JNJVM_MODULE_H

#include <map>

#include "llvm/Constant.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Value.h"

#include "mvm/JIT.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaTypes.h"

namespace jnjvm {

class CommonClass;
class Class;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaJIT;
class JnjvmModule;
class Signdef;

class LLVMAssessorInfo {
public:
  const llvm::Type* llvmType;
  const llvm::Type* llvmTypePtr;
  llvm::Constant* llvmNullConstant;
  llvm::ConstantInt* sizeInBytesConstant;
  
  static void initialise();
  static std::map<const char, LLVMAssessorInfo> AssessorInfo;

};


class LLVMCommonClassInfo : public mvm::JITInfo {
  
  friend class JnjvmModule;

protected:
  CommonClass* classDef;

private:
  /// varGV - The LLVM global variable representing this class.
  ///
  llvm::GlobalVariable* varGV;

  /// delegateeGV - The LLVM global variable representing the 
  /// java/lang/Class instance of this class.
  llvm::GlobalVariable* delegateeGV;


public:
  llvm::Value* getVar(JavaJIT* jit);
  llvm::Value* getDelegatee(JavaJIT* jit);
  
  LLVMCommonClassInfo(CommonClass* cl) : 
    classDef(cl),
    varGV(0),
    delegateeGV(0)
    {}
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
  
  LLVMClassInfo(CommonClass* cl) : 
    LLVMCommonClassInfo(cl),
    virtualSizeConstant(0),
    staticVarGV(0),
    virtualTableGV(0),
    virtualTracerFunction(0),
    staticTracerFunction(0),
    virtualType(0),
    staticType(0) {}
};

class LLVMMethodInfo : public mvm::JITInfo {
private:
  JavaMethod* methodDef;

  llvm::Function* methodFunction;
  llvm::ConstantInt* offsetConstant;
  const llvm::FunctionType* functionType;
  
public:
  llvm::Function* getMethod();
  llvm::ConstantInt* getOffset();
  const llvm::FunctionType* getFunctionType();
  
  LLVMMethodInfo(JavaMethod* M) : 
    methodDef(M), 
    methodFunction(0),
    offsetConstant(0),
    functionType(0) {}
};

class LLVMFieldInfo : public mvm::JITInfo {
private:
  JavaField* fieldDef;
  
  llvm::ConstantInt* offsetConstant;

public:
  llvm::ConstantInt* getOffset();

  LLVMFieldInfo(JavaField* F) : 
    fieldDef(F), 
    offsetConstant(0) {}
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

#ifdef SERVICE_VM
class LLVMServiceInfo : public mvm::JITInfo {
private:
  ServiceDomain* vm;
  llvm::GlobalVariable* delegateeGV;

public:
  llvm::Value* getDelegatee(JavaJIT* jit);
};
#endif

class LLVMConstantPoolInfo : public mvm::JITInfo {
private:
  JavaConstantPool* ctp;
  llvm::GlobalVariable* delegateeGV;

public:
  llvm::Value* getDelegatee(JavaJIT* jit);

  LLVMConstantPoolInfo(JavaConstantPool* c) :
    ctp(c), delegateeGV(0) {}
};

class LLVMStringInfo : public mvm::JITInfo {
private:
  JavaString* str;
  llvm::GlobalVariable* delegateeGV;

public:
  llvm::Value* getDelegatee(JavaJIT* jit);

  LLVMStringInfo(JavaString* c) :
    str(c), delegateeGV(0) {}
};

class JnjvmModule : public llvm::Module {
  friend class LLVMClassInfo;
private:
  std::map<const CommonClass*, LLVMCommonClassInfo*> classMap;
  std::map<const Signdef*, LLVMSignatureInfo*> signatureMap;
  std::map<const JavaField*, LLVMFieldInfo*> fieldMap;
  std::map<const JavaMethod*, LLVMMethodInfo*> methodMap;
  std::map<const JavaString*, LLVMStringInfo*> stringMap;

#ifdef SERVICE_VM
  std::map<const ServiceDomain*, LLVMServiceInfo*> serviceMap;
  typedef std::map<const ServiceDomain*, LLVMServiceInfo*>::iterator
    class_iterator;
#endif
  
  typedef std::map<const CommonClass*, LLVMCommonClassInfo*>::iterator
    class_iterator;  
  
  typedef std::map<const Signdef*, LLVMSignatureInfo*>::iterator
    signature_iterator;  
  
  typedef std::map<const JavaMethod*, LLVMMethodInfo*>::iterator
    method_iterator;  
  
  typedef std::map<const JavaField*, LLVMFieldInfo*>::iterator
    field_iterator;
  
  typedef std::map<const JavaString*, LLVMStringInfo*>::iterator
    string_iterator;


  static VirtualTable* makeVT(Class* cl, bool stat);
  static VirtualTable* allocateVT(Class* cl, CommonClass::method_iterator meths);


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
  static const llvm::Type* JnjvmType;
  static const llvm::Type* ConstantPoolType;
  
#ifdef WITH_TRACER
  static llvm::Function* MarkAndTraceFunction;
  static const llvm::FunctionType* MarkAndTraceType;
  static llvm::Function* JavaObjectTracerFunction;  
#endif
  
  static llvm::Function* GetSJLJBufferFunction;
  static llvm::Function* InterfaceLookupFunction;
  static llvm::Function* VirtualFieldLookupFunction;
  static llvm::Function* StaticFieldLookupFunction;
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
  static llvm::Function* IsAssignableFromFunction;
  static llvm::Function* ImplementsFunction;
  static llvm::Function* InstantiationOfArrayFunction;
  static llvm::Function* GetDepthFunction;
  static llvm::Function* GetClassInDisplayFunction;
  static llvm::Function* GetDisplayFunction;
  static llvm::Function* AquireObjectFunction;
  static llvm::Function* ReleaseObjectFunction;
  static llvm::Function* GetConstantPoolAtFunction;
#ifdef SERVICE_VM
  static llvm::Function* AquireObjectInSharedDomainFunction;
  static llvm::Function* ReleaseObjectInSharedDomainFunction;
  static llvm::Function* ServiceCallStartFunction;
  static llvm::Function* ServiceCallStopFunction;
#endif
  static llvm::Function* MultiCallNewFunction;

#ifdef MULTIPLE_VM
  static llvm::Function* StringLookupFunction;
  static llvm::Function* GetCtpCacheNodeFunction;
  static llvm::Function* GetCtpClassFunction;
  static llvm::Function* EnveloppeLookupFunction;
  static llvm::Function* GetJnjvmExceptionClassFunction;
  static llvm::Function* GetJnjvmArrayClassFunction;
  static llvm::Function* StaticCtpLookupFunction;
  static llvm::Function* SpecialCtpLookupFunction;
  static llvm::Function* GetArrayClassFunction;
#endif

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

  static llvm::Function* GetLockFunction;
  static llvm::Function* GetThreadIDFunction;
  static llvm::Function* OverflowThinLockFunction;

  static llvm::ConstantInt* OffsetObjectSizeInClassConstant;
  static llvm::ConstantInt* OffsetVTInClassConstant;
  static llvm::ConstantInt* OffsetDepthInClassConstant;
  static llvm::ConstantInt* OffsetDisplayInClassConstant;
  static llvm::ConstantInt* OffsetStatusInClassConstant;
  static llvm::ConstantInt* OffsetCtpInClassConstant;

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


  static void resolveVirtualClass(Class* cl);
  static void resolveStaticClass(Class* cl);
  static void setMethod(JavaMethod* meth, const char* name);
  static void* getMethod(JavaMethod* meth);

  static LLVMSignatureInfo* getSignatureInfo(Signdef* sign) {
    return sign->getInfo<LLVMSignatureInfo>();
  }
  
  static LLVMCommonClassInfo* getClassInfo(CommonClass* cl) {
    if (cl->isArray() || cl->isPrimitive()) {
      return cl->getInfo<LLVMCommonClassInfo>();
    } 
    
    return cl->getInfo<LLVMClassInfo>();
  }

  static LLVMFieldInfo* getFieldInfo(JavaField* field) {
    return field->getInfo<LLVMFieldInfo>();
  }
  
  static LLVMMethodInfo* getMethodInfo(JavaMethod* method) {
    return method->getInfo<LLVMMethodInfo>();
  }

  static LLVMConstantPoolInfo* getConstantPoolInfo(JavaConstantPool* ctp) {
    return ctp->getInfo<LLVMConstantPoolInfo>();
  }
  
  static LLVMAssessorInfo& getTypedefInfo(Typedef* type);
  
  LLVMStringInfo* getStringInfo(JavaString* str);

#ifdef SERVICE_VM
  static LLVMServiceInfo* getServiceInfo(ServiceDomain* service) {
    return service->getInfo<LLVMServiceInfo>();
  }
#endif
  
  explicit JnjvmModule(const std::string &ModuleID);
  void initialise();
};

}

#endif
