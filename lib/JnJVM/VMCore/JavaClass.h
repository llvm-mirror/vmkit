//===-------- JavaClass.h - Java class representation -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CLASS_H
#define JNJVM_JAVA_CLASS_H

#include <vector>

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/ExecutionEngine/GenericValue.h"

#include "types.h"

#include "mvm/Method.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "JavaAccess.h"

namespace jnjvm {

class ArrayUInt8;
class AssessorDesc;
class Enveloppe;
class Class;
class JavaCtpInfo;
class JavaField;
class JavaMethod;
class JavaObject;
class Jnjvm;
class Reader;
class Signdef;
class Typedef;
class UTF8;


typedef enum JavaState {
  hashed = 0, loaded, readed, prepared, resolved, clinitParent, inClinit, ready
}JavaState;


class Attribut : public mvm::Object {
public:
  static VirtualTable* VT;
  const UTF8* name;
  unsigned int start;
  unsigned int  nbb;

  static Attribut* derive(const UTF8* name, unsigned int length,
                          const Reader* reader);
  static Attribut* lookup(const std::vector<Attribut*> * vec,
                          const UTF8* key);
  Reader* toReader(ArrayUInt8* array, Attribut* attr);

  static const UTF8* codeAttribut;
  static const UTF8* exceptionsAttribut;
  static const UTF8* constantAttribut;
  static const UTF8* lineNumberTableAttribut;
  static const UTF8* innerClassesAttribut;
  static const UTF8* sourceFileAttribut;
  
  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);
};

class CommonClass : public mvm::Object {
private:
  llvm::GlobalVariable* _llvmVar;
#ifdef SINGLE_VM
  llvm::GlobalVariable* _llvmDelegatee;
#endif
public:
  static VirtualTable* VT;
  const llvm::Type * virtualType;
  const llvm::Type * staticType;
  const UTF8* name;
  CommonClass * super;
  const UTF8* superUTF8;
  std::vector<const UTF8*> interfacesUTF8;
  std::vector<Class*> interfaces;
  mvm::Lock* lockVar;
  mvm::Cond* condVar;
  std::vector<JavaMethod*> virtualMethods;
  std::vector<JavaMethod*> staticMethods;
  std::vector<JavaField*>  virtualFields;
  std::vector<JavaField*>  staticFields;
  JavaObject* classLoader;
#ifdef SINGLE_VM
  JavaObject* delegatee;
#endif
  std::vector<CommonClass*> display;
  unsigned int dim;
  unsigned int depth;
  bool isArray;
  JavaState status;
  unsigned int access;
  Jnjvm *isolate;


  static const int MaxDisplay;
  static JavaObject* jnjvmClassLoader;

  llvm::GlobalVariable* llvmVar(llvm::Module* compilingModule);
#ifdef SINGLE_VM
  llvm::GlobalVariable* llvmDelegatee();
#endif
  static void printClassName(const UTF8* name, mvm::PrintBuffer* buf);
  void initialise(Jnjvm* isolate, bool array);
  void aquire(); 
  void release();
  void waitClass();
  void broadcastClass();
  bool ownerClass();
  
  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse);
  
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type, bool isStatic,
                           bool recurse);
  
  JavaField* lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse);
  
  JavaField* lookupField(const UTF8* name, const UTF8* type, bool isStatic,
                         bool recurse);


  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);

  bool inheritName(const UTF8* Tname);
  bool isOfTypeName(const UTF8* Tname);
  bool implements(CommonClass* cl);
  bool instantiationOfArray(CommonClass* cl);
  bool subclassOf(CommonClass* cl);
  bool isAssignableFrom(CommonClass* cl);
  JavaObject* getClassDelegatee();
  void initialiseClass();
  void resolveClass(bool doClinit);

#ifdef SINGLE_VM
  bool isReady() {
    return status == ready;
  }
#else
  bool isReady();
#endif

};

class Class : public CommonClass {
private:
  llvm::GlobalVariable* _staticVar;

public:
  static VirtualTable* VT;
  unsigned int minor;
  unsigned int major;
  ArrayUInt8* bytes;
  JavaObject* _staticInstance;
  JavaObject* virtualInstance;
  llvm::Function* virtualTracer;
  llvm::Function* staticTracer;
  mvm::Code* codeVirtualTracer;
  mvm::Code* codeStaticTracer;
  JavaCtpInfo* ctpInfo;
  std::vector<Attribut*> attributs;
  std::vector<Class*> innerClasses;
  Class* outerClass;
  uint32 innerAccess;
  bool innerOuterResolved;
  
  void resolveFields();
  llvm::GlobalVariable* staticVar(llvm::Module* compilingModule);
  
  uint64 virtualSize;
  VirtualTable* virtualVT;
#ifndef SINGLE_VM
  uint64 staticSize;
  VirtualTable* staticVT;
  JavaObject* doNewIsolate();
#endif
  JavaObject* doNew();
  JavaObject* doNewUnknown();
  JavaObject* initialiseObject(JavaObject* obj);
  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);

  JavaObject* operator()();

#ifdef SINGLE_VM
  void setStaticInstance(JavaObject* val) {
    _staticInstance = val;
  }
  JavaObject* staticInstance() {
    return _staticInstance;
  }
#else
  void setStaticInstance(JavaObject* val);
  JavaObject* staticInstance();
  JavaObject* createStaticInstance();
#endif

};


class ClassArray : public CommonClass {
public:
  static VirtualTable* VT;
  CommonClass*  _baseClass;
  AssessorDesc* _funcs;

  void resolveComponent();
  CommonClass* baseClass();
  AssessorDesc* funcs();
  static JavaObject* arrayLoader(Jnjvm* isolate, const UTF8* name,
                                 JavaObject* loader, unsigned int start,
                                 unsigned int end);

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);

  static CommonClass* SuperArray;
  static std::vector<Class*>        InterfacesArray;
  static std::vector<JavaMethod*>   VirtualMethodsArray;
  static std::vector<JavaMethod*>   StaticMethodsArray;
  static std::vector<JavaField*>    VirtualFieldsArray;
  static std::vector<JavaField*>    StaticFieldsArray;
};


class JavaMethod : public mvm::Object {
public:
  static VirtualTable* VT;
  llvm::Function* methPtr;
  unsigned int access;
  Signdef* signature;
  std::vector<Attribut*> attributs;
  std::vector<Enveloppe*> caches;
  Class* classDef;
  const UTF8* name;
  const UTF8* type;
  bool canBeInlined;
  mvm::Code* code;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);

  void* compiledPtr();
  const llvm::FunctionType* llvmType;

  uint32 invokeIntSpecialAP(JavaObject* obj, va_list ap);
  float invokeFloatSpecialAP(JavaObject* obj, va_list ap);
  double invokeDoubleSpecialAP(JavaObject* obj, va_list ap);
  sint64 invokeLongSpecialAP(JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectSpecialAP(JavaObject* obj, va_list ap);
  
  uint32 invokeIntVirtualAP(JavaObject* obj, va_list ap);
  float invokeFloatVirtualAP(JavaObject* obj, va_list ap);
  double invokeDoubleVirtualAP(JavaObject* obj, va_list ap);
  sint64 invokeLongVirtualAP(JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectVirtualAP(JavaObject* obj, va_list ap);
  
  uint32 invokeIntStaticAP(va_list ap);
  float invokeFloatStaticAP(va_list ap);
  double invokeDoubleStaticAP(va_list ap);
  sint64 invokeLongStaticAP(va_list ap);
  JavaObject* invokeJavaObjectStaticAP(va_list ap);
  
  uint32 invokeIntSpecialBuf(JavaObject* obj, void* buf);
  float invokeFloatSpecialBuf(JavaObject* obj, void* buf);
  double invokeDoubleSpecialBuf(JavaObject* obj, void* buf);
  sint64 invokeLongSpecialBuf(JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectSpecialBuf(JavaObject* obj, void* buf);
  
  uint32 invokeIntVirtualBuf(JavaObject* obj, void* buf);
  float invokeFloatVirtualBuf(JavaObject* obj, void* buf);
  double invokeDoubleVirtualBuf(JavaObject* obj, void* buf);
  sint64 invokeLongVirtualBuf(JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectVirtualBuf(JavaObject* obj, void* buf);
  
  uint32 invokeIntStaticBuf(void* buf);
  float invokeFloatStaticBuf(void* buf);
  double invokeDoubleStaticBuf(void* buf);
  sint64 invokeLongStaticBuf(void* buf);
  JavaObject* invokeJavaObjectStaticBuf(void* buf);
  
  uint32 invokeIntSpecial(JavaObject* obj, ...);
  float invokeFloatSpecial(JavaObject* obj, ...);
  double invokeDoubleSpecial(JavaObject* obj, ...);
  sint64 invokeLongSpecial(JavaObject* obj, ...);
  JavaObject* invokeJavaObjectSpecial(JavaObject* obj, ...);
  
  uint32 invokeIntVirtual(JavaObject* obj, ...);
  float invokeFloatVirtual(JavaObject* obj, ...);
  double invokeDoubleVirtual(JavaObject* obj, ...);
  sint64 invokeLongVirtual(JavaObject* obj, ...);
  JavaObject* invokeJavaObjectVirtual(JavaObject* obj, ...);
  
  uint32 invokeIntStatic(...);
  float invokeFloatStatic(...);
  double invokeDoubleStatic(...);
  sint64 invokeLongStatic(...);
  JavaObject* invokeJavaObjectStatic(...);
};

class JavaField : public mvm::Object {
public:
  static VirtualTable* VT;
  llvm::ConstantInt* offset;
  unsigned int access;
  const UTF8* name;
  Typedef* signature;
  const UTF8* type;
  std::vector<Attribut*> attributs;
  Class* classDef;
  uint64 ptrOffset;

  void initField(JavaObject* obj);

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);
  
  llvm::GenericValue operator()(JavaObject* obj = 0);
  void operator()(JavaObject* obj, float val);
  void operator()(JavaObject* obj, double val);
  void operator()(JavaObject* obj, uint32 val);
  void operator()(JavaObject* obj, sint64 val);
  void operator()(JavaObject* obj, JavaObject* val);
  void operator()(float val);
  void operator()(double val);
  void operator()(uint32 val);
  void operator()(sint64 val);
  
  
};


} // end namespace jnjvm

#endif
