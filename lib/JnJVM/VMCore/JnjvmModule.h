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


class LLVMClassInfo : public mvm::JITInfo {
  friend class JnjvmModule;
private:
  CommonClass* classDef;
  /// virtualSizeLLVM - The LLVM constant size of instances of this class.
  ///
  llvm::ConstantInt* virtualSizeConstant;
  llvm::Function* virtualTracerFunction;
  llvm::Function* staticTracerFunction;
  /// virtualType - The LLVM type of instance of this class.
  ///
  const llvm::Type * virtualType;

  /// staticType - The LLVM type of the static instance of this class.
  ///
  const llvm::Type * staticType;
public:
  
  llvm::Value* getVirtualSize();
  llvm::Function* getStaticTracer();
  llvm::Function* getVirtualTracer();
  const llvm::Type* getVirtualType();
  const llvm::Type* getStaticType();
  
  LLVMClassInfo(CommonClass* cl) :
    classDef(cl),
    virtualSizeConstant(0),
    virtualTracerFunction(0),
    staticTracerFunction(0),
    virtualType(0),
    staticType(0) {}
};

class LLVMMethodInfo : public mvm::JITInfo, private llvm::Annotation {
private:
  JavaMethod* methodDef;

  llvm::Function* methodFunction;
  llvm::ConstantInt* offsetConstant;
  const llvm::FunctionType* functionType;
  
public:
  llvm::Function* getMethod();
  llvm::ConstantInt* getOffset();
  const llvm::FunctionType* getFunctionType();
    
  LLVMMethodInfo(JavaMethod* M); 

  static JavaMethod* get(const Function* F);
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

class JnjvmModule : public mvm::MvmModule {
  friend class LLVMClassInfo;
private:
  std::map<const CommonClass*, llvm::GlobalVariable*> nativeClasses;
  std::map<const CommonClass*, llvm::GlobalVariable*> javaClasses;
  std::map<const CommonClass*, llvm::GlobalVariable*> virtualTables;
  std::map<const Class*, llvm::GlobalVariable*> staticInstances;
  std::map<const JavaConstantPool*, llvm::GlobalVariable*> constantPools;
  std::map<const JavaString*, llvm::GlobalVariable*> strings;
  std::map<const Enveloppe*, llvm::GlobalVariable*> enveloppes;
  std::map<const JavaMethod*, llvm::GlobalVariable*> nativeFunctions;

  typedef std::map<const CommonClass*, llvm::GlobalVariable*>::iterator
    native_class_iterator;  
  
  typedef std::map<const CommonClass*, llvm::GlobalVariable*>::iterator
    java_class_iterator;
  
  typedef std::map<const CommonClass*, llvm::GlobalVariable*>::iterator
    virtual_table_iterator;
  
  typedef std::map<const Class*, llvm::GlobalVariable*>::iterator
    static_instance_iterator;  
  
  typedef std::map<const JavaConstantPool*, llvm::GlobalVariable*>::iterator
    constant_pool_iterator;
  
  typedef std::map<const JavaString*, llvm::GlobalVariable*>::iterator
    string_iterator;
  
  typedef std::map<const Enveloppe*, llvm::GlobalVariable*>::iterator
    enveloppe_iterator;
  
  typedef std::map<const JavaMethod*, llvm::GlobalVariable*>::iterator
    native_function_iterator;
  
  
  bool staticCompilation;

  
  llvm::Function* makeTracer(Class* cl, bool stat);
  VirtualTable* makeVT(Class* cl, bool stat);
  VirtualTable* allocateVT(Class* cl, uint32 index);

  
public:

  bool isStaticCompiling() {
    return staticCompilation;
  }

  void setIsStaticCompiling(bool sc) {
    staticCompilation = sc;
  }

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
  llvm::Function* MarkAndTraceFunction;
  static const llvm::FunctionType* MarkAndTraceType;
  llvm::Function* JavaObjectTracerFunction;  
#endif
  
  llvm::Function* GetSJLJBufferFunction;
  llvm::Function* InterfaceLookupFunction;
  llvm::Function* VirtualFieldLookupFunction;
  llvm::Function* StaticFieldLookupFunction;
  llvm::Function* PrintExecutionFunction;
  llvm::Function* PrintMethodStartFunction;
  llvm::Function* PrintMethodEndFunction;
  llvm::Function* JniProceedPendingExceptionFunction;
  llvm::Function* InitialiseClassFunction;
  llvm::Function* InitialisationCheckFunction;
  llvm::Function* ForceInitialisationCheckFunction;
  llvm::Function* ClassLookupFunction;
#ifndef WITHOUT_VTABLE
  llvm::Function* VirtualLookupFunction;
#endif
  llvm::Function* InstanceOfFunction;
  llvm::Function* IsAssignableFromFunction;
  llvm::Function* ImplementsFunction;
  llvm::Function* InstantiationOfArrayFunction;
  llvm::Function* GetDepthFunction;
  llvm::Function* GetClassInDisplayFunction;
  llvm::Function* GetStaticInstanceFunction;
  llvm::Function* GetDisplayFunction;
  llvm::Function* AquireObjectFunction;
  llvm::Function* ReleaseObjectFunction;
  llvm::Function* GetConstantPoolAtFunction;
  llvm::Function* MultiCallNewFunction;

#ifdef ISOLATE
  llvm::Function* StringLookupFunction;
#ifdef ISOLATE_SHARING
  llvm::Function* GetCtpCacheNodeFunction;
  llvm::Function* GetCtpClassFunction;
  llvm::Function* EnveloppeLookupFunction;
  llvm::Function* GetJnjvmExceptionClassFunction;
  llvm::Function* GetJnjvmArrayClassFunction;
  llvm::Function* StaticCtpLookupFunction;
  llvm::Function* SpecialCtpLookupFunction;
  llvm::Function* GetArrayClassFunction;
#endif
#endif

  llvm::Function* GetClassDelegateeFunction;
  llvm::Function* ArrayLengthFunction;
  llvm::Function* GetVTFunction;
  llvm::Function* GetClassFunction;
  llvm::Function* JavaObjectAllocateFunction;
  llvm::Function* GetVTFromClassFunction;
  llvm::Function* GetObjectSizeFromClassFunction;

  llvm::Function* GetLockFunction;
  llvm::Function* OverflowThinLockFunction;

  static llvm::ConstantInt* OffsetObjectSizeInClassConstant;
  static llvm::ConstantInt* OffsetVTInClassConstant;
  static llvm::ConstantInt* OffsetDepthInClassConstant;
  static llvm::ConstantInt* OffsetDisplayInClassConstant;
  static llvm::ConstantInt* OffsetStatusInClassConstant;
  static llvm::ConstantInt* OffsetTaskClassMirrorInClassConstant;
  static llvm::ConstantInt* OffsetStaticInstanceInTaskClassMirrorConstant;
  static llvm::ConstantInt* OffsetStatusInTaskClassMirrorConstant;
  
  static llvm::ConstantInt* ClassReadyConstant;

  static llvm::Constant* JavaClassNullConstant;

  static llvm::Constant*    JavaObjectNullConstant;
  static llvm::Constant*    UTF8NullConstant;
  static llvm::Constant*    MaxArraySizeConstant;
  static llvm::Constant*    JavaObjectSizeConstant;
  static llvm::Constant*    JavaArraySizeConstant;

  llvm::Function* GetExceptionFunction;
  llvm::Function* GetJavaExceptionFunction;
  llvm::Function* ThrowExceptionFunction;
  llvm::Function* ClearExceptionFunction;
  llvm::Function* CompareExceptionFunction;
  llvm::Function* NullPointerExceptionFunction;
  llvm::Function* IndexOutOfBoundsExceptionFunction;
  llvm::Function* ClassCastExceptionFunction;
  llvm::Function* OutOfMemoryErrorFunction;
  llvm::Function* NegativeArraySizeExceptionFunction;

  static void resolveVirtualClass(Class* cl);
  static void resolveStaticClass(Class* cl);
  static void setMethod(JavaMethod* meth, const char* name);
  static void* getMethod(JavaMethod* meth);

  static LLVMSignatureInfo* getSignatureInfo(Signdef* sign) {
    return sign->getInfo<LLVMSignatureInfo>();
  }
  
  static LLVMClassInfo* getClassInfo(Class* cl) {
    return cl->getInfo<LLVMClassInfo>();
  }

  static LLVMFieldInfo* getFieldInfo(JavaField* field) {
    return field->getInfo<LLVMFieldInfo>();
  }
  
  static LLVMMethodInfo* getMethodInfo(JavaMethod* method) {
    return method->getInfo<LLVMMethodInfo>();
  }

  static LLVMAssessorInfo& getTypedefInfo(Typedef* type);
  
  explicit JnjvmModule(const std::string &ModuleID, bool sc = false);
  void initialise();

  llvm::Value* getNativeClass(CommonClass* cl);
  llvm::Value* getJavaClass(CommonClass* cl);
  llvm::Value* getStaticInstance(Class* cl);
  llvm::Value* getVirtualTable(CommonClass* cl);
  
  llvm::Value* getEnveloppe(Enveloppe* enveloppe);
  llvm::Value* getString(JavaString* str);
  llvm::Value* getConstantPool(JavaConstantPool* ctp);
  llvm::Value* getNativeFunction(JavaMethod* meth, void* natPtr);

private:
  static llvm::Module* initialModule;

};

}

#endif
