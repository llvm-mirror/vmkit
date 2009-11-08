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
#include <vector>

#include "mvm/Allocator.h"
#include "mvm/JIT.h"
#include "mvm/UTF8.h"

#include "JavaCompiler.h"

namespace llvm {
  class BasicBlock;
  class Constant;
  class ConstantInt;
  class Function;
  class FunctionPassManager;
  class FunctionType;
  class GlobalVariable;
  class GCFunctionInfo;
  class Module;
  class Type;
  class Value;
}

namespace jnjvm {

class ArrayObject;
class Attribut;
class CacheNode;
class CommonClass;
class Class;
class ClassArray;
class ClassPrimitive;
class Enveloppe;
class JavaConstantPool;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaString;
class JavaVirtualTable;
class Jnjvm;
class JnjvmClassLoader;
class JnjvmModule;
class Typedef;
class Signdef;

using mvm::UTF8;

class LLVMAssessorInfo {
public:
  const llvm::Type* llvmType;
  const llvm::Type* llvmTypePtr;
  uint8_t logSizeInBytesConstant;
  
  static void initialise();
  static std::map<const char, LLVMAssessorInfo> AssessorInfo;

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
  
  
public:
  llvm::GCFunctionInfo* GCInfo;
  llvm::Function* getMethod();
  llvm::Constant* getOffset();
  const llvm::FunctionType* getFunctionType();
    
  LLVMMethodInfo(JavaMethod* M) :  methodDef(M), methodFunction(0),
    offsetConstant(0), functionType(0), GCInfo(0) {}
 

  virtual void clear() {
    GCInfo = 0;
    methodFunction = 0;
    offsetConstant = 0;
    functionType = 0;
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

public:

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
  static const llvm::Type* MutatorThreadType;
  
#ifdef ISOLATE_SHARING
  static const llvm::Type* JnjvmType;
#endif
  
  llvm::Function* EmptyTracerFunction;
  llvm::Function* JavaObjectTracerFunction;
  llvm::Function* JavaArrayTracerFunction;
  llvm::Function* ArrayObjectTracerFunction;
  llvm::Function* RegularObjectTracerFunction;
  
  llvm::Function* StartJNIFunction;
  llvm::Function* EndJNIFunction;
  llvm::Function* InterfaceLookupFunction;
  llvm::Function* VirtualFieldLookupFunction;
  llvm::Function* StaticFieldLookupFunction;
  llvm::Function* PrintExecutionFunction;
  llvm::Function* PrintMethodStartFunction;
  llvm::Function* PrintMethodEndFunction;
  llvm::Function* InitialiseClassFunction;
  llvm::Function* InitialisationCheckFunction;
  llvm::Function* ForceInitialisationCheckFunction;
  llvm::Function* ForceLoadedCheckFunction;
  llvm::Function* ClassLookupFunction;
  llvm::Function* StringLookupFunction;
#ifndef WITHOUT_VTABLE
  llvm::Function* VirtualLookupFunction;
#endif
  llvm::Function* IsAssignableFromFunction;
  llvm::Function* IsSecondaryClassFunction;
  llvm::Function* GetDepthFunction;
  llvm::Function* GetDisplayFunction;
  llvm::Function* GetVTInDisplayFunction;
  llvm::Function* GetStaticInstanceFunction;
  llvm::Function* AquireObjectFunction;
  llvm::Function* ReleaseObjectFunction;
  llvm::Function* GetConstantPoolAtFunction;
  llvm::Function* MultiCallNewFunction;
  llvm::Function* GetArrayClassFunction;

#ifdef ISOLATE_SHARING
  llvm::Function* GetCtpCacheNodeFunction;
  llvm::Function* GetCtpClassFunction;
  llvm::Function* EnveloppeLookupFunction;
  llvm::Function* GetJnjvmExceptionClassFunction;
  llvm::Function* GetJnjvmArrayClassFunction;
  llvm::Function* StaticCtpLookupFunction;
  llvm::Function* SpecialCtpLookupFunction;
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
  llvm::Function* GetVTFromClassArrayFunction;
  llvm::Function* GetVTFromCommonClassFunction;
  llvm::Function* GetObjectSizeFromClassFunction;
  llvm::Function* GetBaseClassVTFromVTFunction;

  llvm::Function* GetLockFunction;
  llvm::Function* OverflowThinLockFunction;
  
  llvm::Function* GetFinalInt8FieldFunction;
  llvm::Function* GetFinalInt16FieldFunction;
  llvm::Function* GetFinalInt32FieldFunction;
  llvm::Function* GetFinalLongFieldFunction;
  llvm::Function* GetFinalFloatFieldFunction;
  llvm::Function* GetFinalDoubleFieldFunction;
  llvm::Function* GetFinalObjectFieldFunction;
  
  llvm::Constant* JavaArraySizeOffsetConstant;
  llvm::Constant* JavaArrayElementsOffsetConstant;
  llvm::Constant* JavaObjectLockOffsetConstant;
  llvm::Constant* JavaObjectVTOffsetConstant;

  llvm::Constant* OffsetObjectSizeInClassConstant;
  llvm::Constant* OffsetVTInClassConstant;
  llvm::Constant* OffsetTaskClassMirrorInClassConstant;
  llvm::Constant* OffsetStaticInstanceInTaskClassMirrorConstant;
  llvm::Constant* OffsetInitializedInTaskClassMirrorConstant;
  llvm::Constant* OffsetStatusInTaskClassMirrorConstant;
  
  llvm::Constant* OffsetDoYieldInThreadConstant;
  llvm::Constant* OffsetIsolateInThreadConstant;
  llvm::Constant* OffsetJNIInThreadConstant;
  llvm::Constant* OffsetJavaExceptionInThreadConstant;
  llvm::Constant* OffsetCXXExceptionInThreadConstant;
  
  llvm::Constant* OffsetClassInVTConstant;
  llvm::Constant* OffsetDepthInVTConstant;
  llvm::Constant* OffsetDisplayInVTConstant;
  llvm::Constant* OffsetBaseClassVTInVTConstant;
  
  llvm::Constant* ClassReadyConstant;

  llvm::Constant* JavaObjectNullConstant;
  llvm::Constant* MaxArraySizeConstant;
  llvm::Constant* JavaArraySizeConstant;

  llvm::Function* ThrowExceptionFunction;
  llvm::Function* NullPointerExceptionFunction;
  llvm::Function* IndexOutOfBoundsExceptionFunction;
  llvm::Function* ClassCastExceptionFunction;
  llvm::Function* OutOfMemoryErrorFunction;
  llvm::Function* StackOverflowErrorFunction;
  llvm::Function* NegativeArraySizeExceptionFunction;
  llvm::Function* ArrayStoreExceptionFunction;
  llvm::Function* ArithmeticExceptionFunction;
  llvm::Function* ThrowExceptionFromJITFunction;
  

  JnjvmModule(llvm::Module*);
  
  static void initialise();

};

class JavaLLVMCompiler : public JavaCompiler {
  friend class LLVMClassInfo;
  friend class LLVMMethodInfo;


protected:

  llvm::Module* TheModule;
  llvm::ModuleProvider* TheModuleProvider;
  JnjvmModule JavaIntrinsics;

  void addJavaPasses();

private: 
  
  
  bool enabledException;
  bool cooperativeGC;
  
  virtual void makeVT(Class* cl) = 0;
  
  std::map<llvm::Function*, JavaMethod*> functions;  
  typedef std::map<llvm::Function*, JavaMethod*>::iterator function_iterator;
  
public:

  JavaLLVMCompiler(const std::string &ModuleID);
  
  virtual bool isStaticCompiling() = 0;
  virtual bool emitFunctionName() = 0;

  llvm::Module* getLLVMModule() {
    return TheModule;
  }

  JnjvmModule* getIntrinsics() {
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

  void resolveVirtualClass(Class* cl);
  void resolveStaticClass(Class* cl);
  static llvm::Function* getMethod(JavaMethod* meth);

  static LLVMSignatureInfo* getSignatureInfo(Signdef* sign);
  static LLVMClassInfo* getClassInfo(Class* cl);
  static LLVMFieldInfo* getFieldInfo(JavaField* field);
  static LLVMMethodInfo* getMethodInfo(JavaMethod* method);
  static LLVMAssessorInfo& getTypedefInfo(const Typedef* type);
  

  virtual llvm::Constant* getFinalObject(JavaObject* obj) = 0;
  virtual JavaObject* getFinalObject(llvm::Value* C) = 0;
  virtual llvm::Constant* getNativeClass(CommonClass* cl) = 0;
  virtual llvm::Constant* getJavaClass(CommonClass* cl) = 0;
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl) = 0;
  virtual llvm::Constant* getStaticInstance(Class* cl) = 0;
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*) = 0;
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth) = 0;
  
  virtual llvm::Constant* getEnveloppe(Enveloppe* enveloppe) = 0;
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
  
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat) = 0;
  
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

  llvm::Function* NativeLoader;

};

struct CallbackInfo {
  Class* cl;
  uint16 index;
  bool stat;

  CallbackInfo(Class* c, uint32 i, bool s) :
    cl(c), index(i), stat(s) {}

};

class JavaJITStackScanner : public mvm::JITStackScanner {
public:
  virtual llvm::GCFunctionInfo* IPToGCFunctionInfo(mvm::VirtualMachine* vm,
                                                   void* ip);
};

class JavaJITMethodInfo : public mvm::JITMethodInfo {
protected:
  JavaMethod* meth;
public:
  virtual void print(void* ip, void* addr);
  
  JavaJITMethodInfo(llvm::GCFunctionInfo* GFI, JavaMethod* m) : 
    mvm::JITMethodInfo(GFI) {
    meth = m;
  }
  
  virtual void* getMetaInfo() {
    return meth;
  }

};

class JavaJITCompiler : public JavaLLVMCompiler {
public:

  std::map<llvm::Function*, CallbackInfo> callbacks;

  typedef std::map<llvm::Function*, CallbackInfo>::iterator callback_iterator;


  bool EmitFunctionName;

  JavaJITCompiler(const std::string &ModuleID);
  
  virtual bool isStaticCompiling() {
    return false;
  }
 
  virtual bool emitFunctionName() {
    return EmitFunctionName;
  }

  virtual void makeVT(Class* cl);
  
  virtual JavaCompiler* Create(const std::string& ModuleID) {
    return new JavaJITCompiler(ModuleID);
  }
  
  virtual void* materializeFunction(JavaMethod* meth);
  
  virtual llvm::Constant* getFinalObject(JavaObject* obj);
  virtual JavaObject* getFinalObject(llvm::Value* C);
  virtual llvm::Constant* getNativeClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl);
  virtual llvm::Constant* getStaticInstance(Class* cl);
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*);
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth);
  
  virtual llvm::Constant* getEnveloppe(Enveloppe* enveloppe);
  virtual llvm::Constant* getString(JavaString* str);
  virtual llvm::Constant* getStringPtr(JavaString** str);
  virtual llvm::Constant* getConstantPool(JavaConstantPool* ctp);
  virtual llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr);
  
  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name);
  

#ifdef SERVICE
  virtual llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where);
#endif

  virtual ~JavaJITCompiler() {}
  
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat);

#ifdef WITH_LLVM_GCC
  virtual mvm::StackScanner* createStackScanner() {
    if (useCooperativeGC())
      return new JavaJITStackScanner();
    
    return new mvm::UnpreciseStackScanner();
  }
#endif
  
  virtual void* loadMethod(void* handle, const char* symbol);

};

class JavaAOTCompiler : public JavaLLVMCompiler {

public:
  JavaAOTCompiler(const std::string &ModuleID);
  
  virtual bool isStaticCompiling() {
    return true;
  }
  
  virtual bool emitFunctionName() {
    return true;
  }
  
  virtual JavaCompiler* Create(const std::string& ModuleID) {
    return new JavaAOTCompiler(ModuleID);
  }
  
  virtual void* materializeFunction(JavaMethod* meth) {
    fprintf(stderr, "Can not materiale a function in AOT mode.");
    abort();
  }
  
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat);
  
  virtual void makeVT(Class* cl);
  
  virtual llvm::Constant* getFinalObject(JavaObject* obj);
  virtual JavaObject* getFinalObject(llvm::Value* C);
  virtual llvm::Constant* getNativeClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl);
  virtual llvm::Constant* getStaticInstance(Class* cl);
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*);
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth);
  
  virtual llvm::Constant* getEnveloppe(Enveloppe* enveloppe);
  virtual llvm::Constant* getString(JavaString* str);
  virtual llvm::Constant* getStringPtr(JavaString** str);
  virtual llvm::Constant* getConstantPool(JavaConstantPool* ctp);
  virtual llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr);
  
  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name);
  

#ifdef SERVICE
  virtual llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where);
#endif
  
  virtual ~JavaAOTCompiler() {}
  
  virtual void* loadMethod(void* handle, const char* symbol);

private:

  //--------------- Static compiler specific functions -----------------------//
  llvm::Constant* CreateConstantFromVT(JavaVirtualTable* VT);
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
  llvm::Constant* CreateConstantForBaseObject(CommonClass* cl);
  llvm::Constant* CreateConstantFromJavaObject(JavaObject* obj);
  llvm::Constant* getUTF8(const UTF8* val);
  
  template<typename T>
  llvm::Constant* CreateConstantFromIntArray(const T* val, const llvm::Type* Ty);
  
  template<typename T>
  llvm::Constant* CreateConstantFromFPArray(const T* val, const llvm::Type* Ty);

  llvm::Constant* CreateConstantFromObjectArray(const ArrayObject* val);
  
  std::map<const CommonClass*, llvm::Constant*> nativeClasses;
  std::map<const ClassArray*, llvm::GlobalVariable*> arrayClasses;
  std::map<const CommonClass*, llvm::Constant*> javaClasses;
  std::map<const JavaVirtualTable*, llvm::Constant*> virtualTables;
  std::map<const Class*, llvm::Constant*> staticInstances;
  std::map<const JavaConstantPool*, llvm::Constant*> constantPools;
  std::map<const JavaString*, llvm::Constant*> strings;
  std::map<const Enveloppe*, llvm::Constant*> enveloppes;
  std::map<const JavaMethod*, llvm::Constant*> nativeFunctions;
  std::map<const UTF8*, llvm::Constant*> utf8s;
  std::map<const Class*, llvm::Constant*> virtualMethods;
  std::map<const JavaObject*, llvm::Constant*> finalObjects;
  std::map<const llvm::Constant*, JavaObject*> reverseFinalObjects;
  
  typedef std::map<const JavaObject*, llvm::Constant*>::iterator
    final_object_iterator;
  
  typedef std::map<const llvm::Constant*, JavaObject*>::iterator
    reverse_final_object_iterator;
  
  typedef std::map<const Class*, llvm::Constant*>::iterator
    method_iterator;
  
  typedef std::map<const CommonClass*, llvm::Constant*>::iterator
    native_class_iterator; 
  
  typedef std::map<const ClassArray*, llvm::GlobalVariable*>::iterator
    array_class_iterator;
  
  typedef std::map<const CommonClass*, llvm::Constant*>::iterator
    java_class_iterator;
  
  typedef std::map<const JavaVirtualTable*, llvm::Constant*>::iterator
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

#ifdef SERVICE
  virtual llvm::Value* getIsolate(Jnjvm* vm, llvm::Value* Where);
  std::map<const Jnjvm*, llvm::GlobalVariable*> isolates;
  typedef std::map<const Jnjvm*, llvm::GlobalVariable*>::iterator
    isolate_iterator; 
#endif
  
  bool isCompiling(const CommonClass* cl) const;

public:
  llvm::Function* StaticInitializer;
  llvm::Function* ObjectPrinter;
  llvm::Function* Callback;
  
  bool generateStubs;
  bool assumeCompiled;
  bool compileRT;

  std::vector<std::string>* clinits;
  
  
  void CreateStaticInitializer();
  
  static void setNoInline(Class* cl);
  
  void printStats();
  
  void compileFile(Jnjvm* vm, const char* name);
  void compileClass(Class* cl);
  void generateMain(const char* name, bool jit);

private:
  void compileAllStubs(Signdef* sign);
};

}

#endif
