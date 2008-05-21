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


class Attribut {
public:
  const UTF8* name;
  unsigned int start;
  unsigned int  nbb;

  void derive(const UTF8* name, unsigned int length, const Reader* reader);
  Reader* toReader(Jnjvm *vm, ArrayUInt8* array, Attribut* attr);

  static const UTF8* codeAttribut;
  static const UTF8* exceptionsAttribut;
  static const UTF8* constantAttribut;
  static const UTF8* lineNumberTableAttribut;
  static const UTF8* innerClassesAttribut;
  static const UTF8* sourceFileAttribut;
  
};

class CommonClass : public mvm::Object {
public:
  
  /// virtualSize - The size of instances of this class.
  ///
  uint32 virtualSize;

  /// virtualVT - The virtual table of instances of this class.
  ///
  VirtualTable* virtualVT;
  
  /// virtualTableSize - The size of the virtual table of this class.
  ///
  uint32 virtualTableSize;
  
  /// access - {public, private, protected}.
  ///
  uint32 access;
  
  /// isArray - Is the class an array class?
  ///
  uint8 isArray;
  
  /// name - The name of the class.
  ///
  const UTF8* name;
  
  /// isolate - Which isolate defined this class.
  ///
  Jnjvm *isolate;
  
  /// status - The loading/resolve/initialization state of the class.
  ///
  JavaState status;
  
  /// super - The parent of this class.
  ///
  CommonClass * super;

  /// superUTF8 - The name of the parent of this class.
  ///
  const UTF8* superUTF8;
  
  /// interfaces - The interfaces this class implements.
  ///
  std::vector<Class*> interfaces;

  /// interfacesUTF8 - The names of the interfaces this class implements.
  ///
  std::vector<const UTF8*> interfacesUTF8;
  
  /// lockVar - When multiple threads want to load/resolve/initialize a class,
  /// they must be synchronized so that these steps are only performned once
  /// for a given class.
  mvm::Lock* lockVar;

  /// condVar - Used to wake threads waiting on the load/resolve/initialize
  /// process of this class, done by another thread.
  mvm::Cond* condVar;
  
  /// classLoader - The Java class loader that loaded the class.
  ///
  JavaObject* classLoader;
  
#ifndef MULTIPLE_VM
  /// delegatee - The java/lang/Class object representing this class
  ///
  JavaObject* delegatee;
#endif
  
  /// virtualMethods - List of all the virtual methods defined by this class.
  /// This does not contain non-redefined super methods.
  std::vector<JavaMethod*> virtualMethods;

  /// staticMethods - List of all the static methods defined by this class.
  ///
  std::vector<JavaMethod*> staticMethods;

  /// virtualFields - List of all the virtual fields defined in this class.
  /// This does not contain non-redefined super fields.
  std::vector<JavaField*>  virtualFields;

  /// staticFields - List of all the static fields defined in this class.
  ///
  std::vector<JavaField*>  staticFields;
  
  /// display - The class hierarchy of supers for this class.
  ///
  std::vector<CommonClass*> display;

  /// depth - The depth of this class in its class hierarchy. 
  /// display[depth - 1] contains the class.
  uint32 depth;
  
  static void printClassName(const UTF8* name, mvm::PrintBuffer* buf);
  void initialise(Jnjvm* isolate, bool array);
  
  void aquire() {
    lockVar->lock();
  }
  
  void release() {
    lockVar->unlock();  
  }

  void waitClass() {
    condVar->wait(lockVar);
  }

  void broadcastClass() {
    condVar->broadcast();  
  }

  bool ownerClass() {
    return mvm::Lock::selfOwner(lockVar);    
  }
  
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
  
  static VirtualTable* VT;
  static const int MaxDisplay;
  static JavaObject* jnjvmClassLoader;
};

class Class : public CommonClass {
public:
  static VirtualTable* VT;
  unsigned int minor;
  unsigned int major;
  ArrayUInt8* bytes;
  mvm::Code* codeVirtualTracer;
  mvm::Code* codeStaticTracer;
  JavaCtpInfo* ctpInfo;
  std::vector<Attribut*> attributs;
  std::vector<Class*> innerClasses;
  Class* outerClass;
  uint32 innerAccess;
  bool innerOuterResolved;
  
  uint32 staticSize;
  VirtualTable* staticVT;
  JavaObject* doNew(Jnjvm* vm);
  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  virtual void destroyer(size_t sz);

  JavaObject* operator()(Jnjvm* vm);
  
  Attribut* lookupAttribut(const UTF8* key);

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
  
  CommonClass* baseClass() {
    if (_baseClass == 0)
      resolveComponent();
    return _baseClass;
  }

  AssessorDesc* funcs() {
    if (_funcs == 0)
      resolveComponent();
    return _funcs;
  }

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
private:
  void* _compiledPtr();
public:
  static VirtualTable* VT;
  unsigned int access;
  Signdef* signature;
  std::vector<Attribut*> attributs;
  std::vector<Enveloppe*, gc_allocator<Enveloppe*> > caches;
  Class* classDef;
  const UTF8* name;
  const UTF8* type;
  bool canBeInlined;
  mvm::Code* code;
  
  /// offset - The index of the method in the virtual table.
  ///
  uint32 offset;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  
  Attribut* lookupAttribut(const UTF8* key);

  void* compiledPtr() {
    if (!code) return _compiledPtr();
    return code;
  }

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
  unsigned int access;
  const UTF8* name;
  Typedef* signature;
  const UTF8* type;
  std::vector<Attribut*> attributs;
  Class* classDef;
  uint64 ptrOffset;
  /// num - The index of the field in the field list.
  ///
  uint32 num;

  void initField(JavaObject* obj);
  Attribut* lookupAttribut(const UTF8* key);

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  
  #define GETVIRTUALFIELD(TYPE, TYPE_NAME) \
  TYPE getVirtual##TYPE_NAME##Field(JavaObject* obj) { \
    assert(classDef->isReady()); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  #define GETSTATICFIELD(TYPE, TYPE_NAME) \
  TYPE getStatic##TYPE_NAME##Field() { \
    assert(classDef->isReady()); \
    JavaObject* obj = classDef->staticInstance(); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  #define SETVIRTUALFIELD(TYPE, TYPE_NAME) \
  void setVirtual##TYPE_NAME##Field(JavaObject* obj, TYPE val) { \
    assert(classDef->isReady()); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    ((TYPE*)ptr)[0] = val; \
  }

  #define SETSTATICFIELD(TYPE, TYPE_NAME) \
  void setStatic##TYPE_NAME##Field(TYPE val) { \
    assert(classDef->isReady()); \
    JavaObject* obj = classDef->staticInstance(); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    ((TYPE*)ptr)[0] = val; \
  }

  #define MK_ASSESSORS(TYPE, TYPE_NAME) \
    GETVIRTUALFIELD(TYPE, TYPE_NAME) \
    SETVIRTUALFIELD(TYPE, TYPE_NAME) \
    GETSTATICFIELD(TYPE, TYPE_NAME) \
    SETSTATICFIELD(TYPE, TYPE_NAME) \

  MK_ASSESSORS(float, Float);
  MK_ASSESSORS(double, Double);
  MK_ASSESSORS(JavaObject*, Object);
  MK_ASSESSORS(uint8, Int8);
  MK_ASSESSORS(uint16, Int16);
  MK_ASSESSORS(uint32, Int32);
  MK_ASSESSORS(sint64, Long);

};


} // end namespace jnjvm

#endif
