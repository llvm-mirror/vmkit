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

#include "mvm/JIT.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaTypes.h"

#include "llvm/Module.h"

namespace llvm {
  class Constant;
  class ConstantInt;
  class Function;
  class GlobalVariable;
  class Type;
  class Value;
}

namespace jnjvm {

class Attribut;
class CacheNode;
class CommonClass;
class Class;
class Enveloppe;
class JavaField;
class JavaMethod;
class JavaObject;
class JnjvmModule;

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
  Class* classDef;
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
  
  LLVMClassInfo(Class* cl) :
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

  static JavaMethod* get(const llvm::Function* F);
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
  std::map<const CommonClass*, llvm::Constant*> nativeClasses;
  std::map<const ClassArray*, llvm::GlobalVariable*> arrayClasses;
  std::map<const CommonClass*, llvm::Constant*> javaClasses;
  std::map<const CommonClass*, llvm::Constant*> virtualTables;
  std::map<const Class*, llvm::Constant*> staticInstances;
  std::map<const JavaConstantPool*, llvm::Constant*> constantPools;
  std::map<const JavaString*, llvm::Constant*> strings;
  std::map<const Enveloppe*, llvm::Constant*> enveloppes;
  std::map<const JavaMethod*, llvm::Constant*> nativeFunctions;
  std::map<const UTF8*, llvm::Constant*> utf8s;
  std::map<const Class*, llvm::Constant*> virtualMethods;
  
  typedef std::map<const Class*, llvm::Constant*>::iterator
    method_iterator;
  
  typedef std::map<const CommonClass*, llvm::Constant*>::iterator
    native_class_iterator; 
  
  typedef std::map<const ClassArray*, llvm::GlobalVariable*>::iterator
    array_class_iterator;
  
  typedef std::map<const CommonClass*, llvm::Constant*>::iterator
    java_class_iterator;
  
  typedef std::map<const CommonClass*, llvm::Constant*>::iterator
    virtual_table_iterator;
  
  typedef std::map<const Class*, llvm::Constant*>::iterator
    static_instance_iterator;
  
  typedef std::map<const JavaConstantPool*, llvm::Constant*>::iterator
    constant_pool_iterator;
  
  typedef std::map<const JavaString*, llvm::Constant*>::iterator
    string_iterator;
  
  typedef std::map<const Enveloppe*, llvm::Constant*>::iterator
    enveloppe_iterator;
  
  typedef std::map<const JavaMethod*, llvm::Constant*>::iterator
    native_function_iterator;
  
  typedef std::map<const UTF8*, llvm::Constant*>::iterator
    utf8_iterator;
  
  
  bool staticCompilation;

  
  llvm::Function* makeTracer(Class* cl, bool stat);
  void makeVT(Class* cl);
  void allocateVT(Class* cl);
  
  static llvm::Constant* PrimitiveArrayVT;
  static llvm::Constant* ReferenceArrayVT;
  
  static llvm::Function* StaticInitializer;
  static llvm::Function* ObjectPrinter;
  
public:
  
  static llvm::Function* NativeLoader;

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
  static const llvm::Type* JavaCommonClassType;
  static const llvm::Type* JavaClassType;
  static const llvm::Type* JavaClassArrayType;
  static const llvm::Type* JavaClassPrimitiveType;
  static const llvm::Type* JavaCacheType;
  static const llvm::Type* EnveloppeType;
  static const llvm::Type* CacheNodeType;
  static const llvm::Type* ConstantPoolType;
  static const llvm::Type* UTF8Type;
  static const llvm::Type* JavaMethodType;
  static const llvm::Type* JavaFieldType;
  static const llvm::Type* AttributType;
  static const llvm::Type* JavaThreadType;
  
#ifdef ISOLATE_SHARING
  static const llvm::Type* JnjvmType;
#endif
  
#ifdef WITH_TRACER
  llvm::Function* MarkAndTraceFunction;
  static const llvm::FunctionType* MarkAndTraceType;
  llvm::Function* JavaObjectTracerFunction;  
  llvm::Function* JavaArrayTracerFunction;  
  llvm::Function* ArrayObjectTracerFunction;  
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
  llvm::Function* GetArrayClassFunction;

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
#endif
#endif

#ifdef SERVICE
  llvm::Function* ServiceCallStartFunction;
  llvm::Function* ServiceCallStopFunction;
#endif

  llvm::Function* GetClassDelegateeFunction;
  llvm::Function* RuntimeDelegateeFunction;
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
  static llvm::ConstantInt* OffsetTaskClassMirrorInClassConstant;
  static llvm::ConstantInt* OffsetStaticInstanceInTaskClassMirrorConstant;
  static llvm::ConstantInt* OffsetStatusInTaskClassMirrorConstant;
  
  static llvm::ConstantInt* ClassReadyConstant;

  static llvm::Constant*    JavaObjectNullConstant;
  static llvm::Constant*    MaxArraySizeConstant;
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
  static void setMethod(JavaMethod* meth, void* ptr, const char* name);
  static llvm::Function* getMethod(JavaMethod* meth);

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

  static LLVMAssessorInfo& getTypedefInfo(const Typedef* type);
  
  explicit JnjvmModule(const std::string &ModuleID, bool sc = false);
  void initialise();
  void printStats();

  llvm::Constant* getNativeClass(CommonClass* cl);
  llvm::Constant* getJavaClass(CommonClass* cl);
  llvm::Constant* getStaticInstance(Class* cl);
  llvm::Constant* getVirtualTable(Class* cl);
  llvm::Constant* getMethodInClass(JavaMethod* meth);
  
  llvm::Constant* getEnveloppe(Enveloppe* enveloppe);
  llvm::Constant* getString(JavaString* str);
  llvm::Constant* getConstantPool(JavaConstantPool* ctp);
  llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr);
  
  llvm::Constant* getReferenceArrayVT();
  llvm::Constant* getPrimitiveArrayVT();

#ifdef SERVICE
  std::map<const Jnjvm*, llvm::GlobalVariable*> isolates;
  typedef std::map<const Jnjvm*, llvm::GlobalVariable*>::iterator
    isolate_iterator;
  
  llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where);
#endif
  

  bool isCompiling(const CommonClass* cl) const;
  
  void CreateStaticInitializer();
  
  static void setNoInline(Class* cl);

private:
  static llvm::Module* initialModule;
  
  //--------------- Static compiler specific functions -----------------------//
  llvm::Constant* CreateConstantFromVT(Class* classDef);
  llvm::Constant* CreateConstantFromUTF8(const UTF8* val);
  llvm::Constant* CreateConstantFromEnveloppe(Enveloppe* val);
  llvm::Constant* CreateConstantFromCacheNode(CacheNode* CN);
  llvm::Constant* CreateConstantFromCommonClass(CommonClass* cl);
  llvm::Constant* CreateConstantFromClass(Class* cl);
  llvm::Constant* CreateConstantFromClassPrimitive(ClassPrimitive* cl);
  llvm::Constant* CreateConstantFromClassArray(ClassArray* cl);
  llvm::Constant* CreateConstantFromAttribut(Attribut& attribut);
  llvm::Constant* CreateConstantFromJavaField(JavaField& field);
  llvm::Constant* CreateConstantFromJavaMethod(JavaMethod& method);
  llvm::Constant* CreateConstantFromStaticInstance(Class* cl);
  llvm::Constant* CreateConstantFromJavaString(JavaString* str);
  llvm::Constant* CreateConstantFromJavaClass(CommonClass* cl);
  llvm::Constant* CreateConstantForJavaObject(CommonClass* cl);
  llvm::Constant* getUTF8(const UTF8* val);

};

}

#endif
