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

#include <map>
#include <vector>

#include "types.h"

#include "mvm/Method.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "JavaAccess.h"
#include "Jnjvm.h"

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

  Attribut(const UTF8* name, unsigned int length, const Reader& reader);

  static const UTF8* codeAttribut;
  static const UTF8* exceptionsAttribut;
  static const UTF8* constantAttribut;
  static const UTF8* lineNumberTableAttribut;
  static const UTF8* innerClassesAttribut;
  static const UTF8* sourceFileAttribut;
  
};

/// CommonClass - This class is the root class of all Java classes. It is
/// currently GC-allocated in JnJVM, but will be permanently allocated when the
/// class loader finalizer method will be defined.
///
class CommonClass : public mvm::Object {
private:


/// FieldCmp - Internal class for field and method lookup in a class.
///
class FieldCmp {
public:
  
  /// name - The name of the field/method
  ///
  const UTF8* name;

  /// type - The type of the field/method.
  ///
  const UTF8* type;

  FieldCmp(const UTF8* n, const UTF8* t) : name(n), type(t) {}
  
  inline bool operator<(const FieldCmp &cmp) const {
    if (name < cmp.name) return true;
    else if (name > cmp.name) return false;
    else return type < cmp.type;
  }
};

public:
  
//===----------------------------------------------------------------------===//
//
// Do not reorder these fields or add new ones! the LLVM runtime assumes that
// classes have the following beginning layout.
//
//===----------------------------------------------------------------------===//

  
  /// virtualSize - The size of instances of this class. Array classes do
  /// not need this information, but to simplify accessing this field in
  /// the JIT, we put this in here.
  /// 
  uint32 virtualSize;

  /// virtualVT - The virtual table of instances of this class. Like the
  /// virtualSize field, array classes do not need this information. But we
  /// simplify JIT generation to set it here.
  ///
  VirtualTable* virtualVT;
  
  /// display - The class hierarchy of supers for this class. Array classes
  /// do not need it.
  ///
  CommonClass** display;
  
  /// depth - The depth of this class in its class hierarchy. 
  /// display[depth] contains the class. Array classes do not need it.
  ///
  uint32 depth;


//===----------------------------------------------------------------------===//
//
// New fields can be added from now, or reordered.
//
//===----------------------------------------------------------------------===//
  

  /// virtualTableSize - The size of the virtual table of this class.
  ///
  uint32 virtualTableSize;
   
  /// access - {public, private, protected}.
  ///
  uint32 access;
  
  /// isArray - Is the class an array class?
  ///
  bool isArray;
  
  /// isPrimitive - Is the class a primitive class?
  ///
  bool isPrimitive;
  
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
  

  typedef std::map<const FieldCmp, JavaField*, std::less<FieldCmp> >::iterator
    field_iterator;
  
  typedef std::map<const FieldCmp, JavaField*, std::less<FieldCmp> > 
    field_map;
  
  /// virtualFields - List of all the virtual fields defined in this class.
  /// This does not contain non-redefined super fields.
  field_map virtualFields;
  
  /// staticFields - List of all the static fields defined in this class.
  ///
  field_map staticFields;
  
  typedef std::map<const FieldCmp, JavaMethod*, std::less<FieldCmp> >::iterator
    method_iterator;
  
  typedef std::map<const FieldCmp, JavaMethod*, std::less<FieldCmp> > 
    method_map;
  
  /// virtualMethods - List of all the virtual methods defined by this class.
  /// This does not contain non-redefined super methods.
  method_map virtualMethods;
  
  /// staticMethods - List of all the static methods defined by this class.
  ///
  method_map staticMethods;
  
  /// constructMethod - Add a new method in this class method map.
  ///
  JavaMethod* constructMethod(const UTF8* name, const UTF8* type,
                              uint32 access);
  
  /// constructField - Add a new field in this class field map.
  ///
  JavaField* constructField(const UTF8* name, const UTF8* type,
                            uint32 access);

  /// printClassName - Adds a string representation of this class in the
  /// given buffer.
  ///
  static void printClassName(const UTF8* name, mvm::PrintBuffer* buf);
  
  /// acquire - Acquire this class lock.
  ///
  void acquire() {
    lockVar->lock();
  }
  
  /// release - Release this class lock.
  ///
  void release() {
    lockVar->unlock();  
  }

  /// waitClass - Wait for the class to be loaded/initialized/resolved.
  ///
  void waitClass() {
    condVar->wait(lockVar);
  }
  
  /// broadcastClass - Unblock threads that were waiting on the class being
  /// loaded/initialized/resolved.
  ///
  void broadcastClass() {
    condVar->broadcast();  
  }

  /// ownerClass - Is the current thread the owner of this thread?
  ///
  bool ownerClass() {
    return mvm::Lock::selfOwner(lockVar);    
  }
  
  /// lookupMethodDontThrow - Lookup a method in the method map of this class.
  /// Do not throw if the method is not found.
  ///
  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse);
  
  /// lookupMethod - Lookup a method and throw an exception if not found.
  ///
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type, bool isStatic,
                           bool recurse);
  
  /// lookupFieldDontThrow - Lookup a field in the field map of this class. Do
  /// not throw if the field is not found.
  ///
  JavaField* lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse);
  
  /// lookupField - Lookup a field and throw an exception if not found.
  ///
  JavaField* lookupField(const UTF8* name, const UTF8* type, bool isStatic,
                         bool recurse);

  /// print - Print the class for debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer *buf) const;
  
  /// tracer - The tracer of this GC-allocated class.
  ///
  virtual void TRACER;

  /// inheritName - Does this class in its class hierarchy inherits
  /// the given name? Equality is on the name. This function does not take
  /// into account array classes.
  ///
  bool inheritName(const UTF8* Tname);

  /// isOfTypeName - Does this class inherits the given name? Equality is on
  /// the name. This function takes into account array classes.
  ///
  bool isOfTypeName(const UTF8* Tname);

  /// implements - Does this class implement the given class? Returns true if
  /// the class is in the interface class hierarchy.
  ///
  bool implements(CommonClass* cl);
  
  /// instantationOfArray - If this class is an array class, does its subclass
  /// implements the given array class subclass?
  ///
  bool instantiationOfArray(ClassArray* cl);
  
  /// subclassOf - If this class is a regular class, is it a subclass of the
  /// given class?
  ///
  bool subclassOf(CommonClass* cl);

  /// isAssignableFrom - Is this class assignable from the given class? The
  /// classes may be of any type.
  ///
  bool isAssignableFrom(CommonClass* cl);

  /// getClassDelegatee - Return the java/lang/Class representation of this
  /// class.
  ///
  JavaObject* getClassDelegatee();

  /// initialiseClass - If the class has a static initializer and has not been
  /// initialized yet, call it.
  void initialiseClass();

  /// resolveClass - If the class has not been resolved yet, resolve it.
  ///
  void resolveClass(bool doClinit);

#ifndef MULTIPLE_VM
  /// getStatus - Get the resolution/initialization status of this class.
  ///
  JavaState* getStatus() {
    return &status;
  }
  /// isReady - Has this class been initialized?
  ///
  bool isReady() {
    return status == ready;
  }
#else
  JavaState* getStatus();
  bool isReady() {
    return *getStatus() == ready;
  }
#endif

  /// isResolved - Has this class been resolved?
  ///
  bool isResolved() {
    return status >= resolved;
  }
  
  /// CommonClass - Create a class with th given name.
  ///
  CommonClass(Jnjvm* vm, const UTF8* name, bool isArray);
  
  /// VT - The virtual table of instances of this class (TODO: should be
  /// removed).
  ///
  static VirtualTable* VT;

  /// jnjvmClassLoader - The bootstrap class loader (null).
  ///
  static JavaObject* jnjvmClassLoader;
  
  /// ~CommonClass - Free memory used by this class, and remove it from
  /// metadata.
  ///
  ~CommonClass();

  /// CommonClass - Default constructor.
  ///
  CommonClass();

};

class ClassPrimitive : public CommonClass {
public:
  ClassPrimitive(Jnjvm* vm, const UTF8* name);
};

class Class : public CommonClass {
public:
  static VirtualTable* VT;
  unsigned int minor;
  unsigned int major;
  ArrayUInt8* bytes;
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
  
  ~Class();
  Class();

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
  
  Class(Jnjvm* vm, const UTF8* name);
  
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
  
  /// Empty constructor for VT
  ClassArray() {}
  ClassArray(Jnjvm* vm, const UTF8* name);

  static JavaObject* arrayLoader(Jnjvm* isolate, const UTF8* name,
                                 JavaObject* loader, unsigned int start,
                                 unsigned int end);

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;

  static CommonClass* SuperArray;
  static std::vector<Class*>        InterfacesArray;
};


class JavaMethod {
  friend class CommonClass;
private:
  void* _compiledPtr();
  Signdef* _signature;
public:
  ~JavaMethod();
  unsigned int access;
  std::vector<Attribut*> attributs;
  std::vector<Enveloppe*> caches;
  Class* classDef;
  const UTF8* name;
  const UTF8* type;
  bool canBeInlined;
  void* code;
  
  /// offset - The index of the method in the virtual table.
  ///
  uint32 offset;

  Attribut* lookupAttribut(const UTF8* key);

  void* compiledPtr() {
    if (!code) return _compiledPtr();
    return code;
  }
  
  Signdef* getSignature() {
    if(!_signature)
      _signature = (Signdef*) classDef->isolate->constructType(type);
    return _signature;
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
  
  const char* printString() const;
};

class JavaField  {
  friend class CommonClass;
private:
  Typedef* _signature;
public:
  ~JavaField();
  unsigned int access;
  const UTF8* name;
  const UTF8* type;
  std::vector<Attribut*> attributs;
  Class* classDef;
  uint64 ptrOffset;
  /// num - The index of the field in the field list.
  ///
  uint32 num;
  
  Typedef* getSignature() {
    if(!_signature)
      _signature = classDef->isolate->constructType(type);
    return _signature;
  }

  void initField(JavaObject* obj);
  Attribut* lookupAttribut(const UTF8* key);
  const char* printString() const;

  #define GETVIRTUALFIELD(TYPE, TYPE_NAME) \
  TYPE getVirtual##TYPE_NAME##Field(JavaObject* obj) { \
    assert(*(classDef->getStatus()) >= inClinit); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  #define GETSTATICFIELD(TYPE, TYPE_NAME) \
  TYPE getStatic##TYPE_NAME##Field() { \
    assert(*(classDef->getStatus()) >= inClinit); \
    JavaObject* obj = classDef->staticInstance(); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  #define SETVIRTUALFIELD(TYPE, TYPE_NAME) \
  void setVirtual##TYPE_NAME##Field(JavaObject* obj, TYPE val) { \
    assert(*(classDef->getStatus()) >= inClinit); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    ((TYPE*)ptr)[0] = val; \
  }

  #define SETSTATICFIELD(TYPE, TYPE_NAME) \
  void setStatic##TYPE_NAME##Field(TYPE val) { \
    assert(*(classDef->getStatus()) >= inClinit); \
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
