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
class JavaJIT;
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

  void derive(const UTF8* name, unsigned int length, const Reader* reader);
  static Attribut* lookup(const std::vector<Attribut*, gc_allocator<Attribut*> > * vec,
                          const UTF8* key);
  Reader* toReader(Jnjvm *vm, ArrayUInt8* array, Attribut* attr);

  static const UTF8* codeAttribut;
  static const UTF8* exceptionsAttribut;
  static const UTF8* constantAttribut;
  static const UTF8* lineNumberTableAttribut;
  static const UTF8* innerClassesAttribut;
  static const UTF8* sourceFileAttribut;
  
  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
};

class CommonClass : public mvm::Object {
public:
  JavaObject* classLoader;
private:
  llvm::GlobalVariable* _llvmVar;
#ifndef MULTIPLE_VM
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
#ifndef MULTIPLE_VM
  JavaObject* delegatee;
#endif
  std::vector<CommonClass*> display;
  unsigned int dim;
  unsigned int depth;
  bool isArray;
  JavaState status;
  unsigned int access;
  Jnjvm *isolate;
  unsigned int virtualTableSize;

  static const int MaxDisplay;
  static JavaObject* jnjvmClassLoader;

  llvm::GlobalVariable* llvmVar(llvm::Module* compilingModule);
  llvm::Value* llvmDelegatee(llvm::Module* M, llvm::BasicBlock* BB);
  
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
  virtual void TRACER;

  bool inheritName(const UTF8* Tname);
  bool isOfTypeName(const UTF8* Tname);
  bool implements(CommonClass* cl);
  bool instantiationOfArray(CommonClass* cl);
  bool subclassOf(CommonClass* cl);
  bool isAssignableFrom(CommonClass* cl);
  JavaObject* getClassDelegatee();
  void initialiseClass();
  void resolveClass(bool doClinit);

#ifndef MULTIPLE_VM
  JavaState* getStatus() {
    return &status;
  }
  bool isReady() {
    return status == ready;
  }
#else
  JavaState* getStatus();
  bool isReady() {
    return *getStatus() == ready;
  }
#endif
  bool isResolved() {
    return status >= resolved;
  }
};

class Class : public CommonClass {
private:
  llvm::GlobalVariable* _staticVar;

public:
  static VirtualTable* VT;
  unsigned int minor;
  unsigned int major;
  ArrayUInt8* bytes;
  JavaObject* virtualInstance;
  llvm::Function* virtualTracer;
  llvm::Function* staticTracer;
  mvm::Code* codeVirtualTracer;
  mvm::Code* codeStaticTracer;
  JavaCtpInfo* ctpInfo;
  std::vector<Attribut*, gc_allocator<Attribut*> > attributs;
  std::vector<Class*> innerClasses;
  Class* outerClass;
  uint32 innerAccess;
  bool innerOuterResolved;
  
  void resolveFields();
  llvm::Value* staticVar(JavaJIT* jit);
  
  uint64 virtualSize;
  VirtualTable* virtualVT;
  uint64 staticSize;
  VirtualTable* staticVT;
  JavaObject* doNew(Jnjvm* vm);
  JavaObject* doNewUnknown(Jnjvm* vm);
  JavaObject* initialiseObject(JavaObject* obj);
  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;

  JavaObject* operator()(Jnjvm* vm);

#ifndef MULTIPLE_VM
  JavaObject* _staticInstance;
  JavaObject* staticInstance() {
    return _staticInstance;
  }
  void createStaticInstance() { }
#else
  JavaObject* staticInstance();
  void createStaticInstance();
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
  virtual void TRACER;

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
  llvm::Function* llvmFunction;
  llvm::ConstantInt* offset;
  unsigned int access;
  Signdef* signature;
  std::vector<Attribut*, gc_allocator<Attribut*> > attributs;
  std::vector<Enveloppe*, gc_allocator<Enveloppe*> > caches;
  Class* classDef;
  const UTF8* name;
  const UTF8* type;
  bool canBeInlined;
  mvm::Code* code;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;

  void* compiledPtr();
  const llvm::FunctionType* llvmType;

  uint32 invokeIntSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  float invokeFloatSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  double invokeDoubleSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  sint64 invokeLongSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  
  uint32 invokeIntVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  float invokeFloatVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  double invokeDoubleVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  sint64 invokeLongVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  
  uint32 invokeIntStaticAP(Jnjvm* vm, va_list ap);
  float invokeFloatStaticAP(Jnjvm* vm, va_list ap);
  double invokeDoubleStaticAP(Jnjvm* vm, va_list ap);
  sint64 invokeLongStaticAP(Jnjvm* vm, va_list ap);
  JavaObject* invokeJavaObjectStaticAP(Jnjvm* vm, va_list ap);
  
  uint32 invokeIntSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  float invokeFloatSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  double invokeDoubleSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  sint64 invokeLongSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  
  uint32 invokeIntVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  float invokeFloatVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  double invokeDoubleVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  sint64 invokeLongVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  
  uint32 invokeIntStaticBuf(Jnjvm* vm, void* buf);
  float invokeFloatStaticBuf(Jnjvm* vm, void* buf);
  double invokeDoubleStaticBuf(Jnjvm* vm, void* buf);
  sint64 invokeLongStaticBuf(Jnjvm* vm, void* buf);
  JavaObject* invokeJavaObjectStaticBuf(Jnjvm* vm, void* buf);
  
  uint32 invokeIntSpecial(Jnjvm* vm, JavaObject* obj, ...);
  float invokeFloatSpecial(Jnjvm* vm, JavaObject* obj, ...);
  double invokeDoubleSpecial(Jnjvm* vm, JavaObject* obj, ...);
  sint64 invokeLongSpecial(Jnjvm* vm, JavaObject* obj, ...);
  JavaObject* invokeJavaObjectSpecial(Jnjvm* vm, JavaObject* obj, ...);
  
  uint32 invokeIntVirtual(Jnjvm* vm, JavaObject* obj, ...);
  float invokeFloatVirtual(Jnjvm* vm, JavaObject* obj, ...);
  double invokeDoubleVirtual(Jnjvm* vm, JavaObject* obj, ...);
  sint64 invokeLongVirtual(Jnjvm* vm, JavaObject* obj, ...);
  JavaObject* invokeJavaObjectVirtual(Jnjvm* vm, JavaObject* obj, ...);
  
  uint32 invokeIntStatic(Jnjvm* vm, ...);
  float invokeFloatStatic(Jnjvm* vm, ...);
  double invokeDoubleStatic(Jnjvm* vm, ...);
  sint64 invokeLongStatic(Jnjvm* vm, ...);
  JavaObject* invokeJavaObjectStatic(Jnjvm* vm, ...);
};

class JavaField : public mvm::Object {
public:
  static VirtualTable* VT;
  llvm::ConstantInt* offset;
  unsigned int access;
  const UTF8* name;
  Typedef* signature;
  const UTF8* type;
  std::vector<Attribut*, gc_allocator<Attribut*> > attributs;
  Class* classDef;
  uint64 ptrOffset;

  void initField(JavaObject* obj);

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  
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
