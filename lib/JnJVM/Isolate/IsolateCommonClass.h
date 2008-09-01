//===---- IsolateCommonClass.h - User visible classes with isolates -------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CLASS_H
#error "Never use <IsolateCommonClass.h> directly, but include <JavaClass.h>"
#endif

#ifndef ISOLATE_COMMON_CLASS_H
#define ISOLATE_COMMON_CLASS_H

#include "mvm/Object.h"

#include "JavaConstantPool.h"

namespace jnjvm {

class ArrayUInt8;
class AssessorDesc;
class CommonClass;
class Class;
class ClassArray;
class ClassPrimitive;
class JavaConstantPool;
class JavaField;
class JavaMethod;
class JavaObject;
class Jnjvm;
class JnjvmClassLoader;
class UserClass;
class UserClassArray;
class UserConstantPool;
class UTF8;
enum JavaState;

class UserCommonClass : public mvm::Object {
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
  UserCommonClass** display;
  
  /// depth - The depth of this class in its class hierarchy. 
  /// display[depth] contains the class. Array classes do not need it.
  ///
  uint32 depth;

  /// status - The loading/resolve/initialization state of the class.
  ///
  JavaState status;
  
  /// ctpInfo - The private constant pool of this class.
  ///
  UserConstantPool* ctpInfo;

//===----------------------------------------------------------------------===//
//
// New fields can be added from now, or reordered.
//
//===----------------------------------------------------------------------===// 

  JnjvmClassLoader* classLoader;
  JavaObject* delegatee;
  CommonClass* classDef;
  UserClass* super;
  std::vector<UserClass*> interfaces;

  bool inheritName(const UTF8* Tname);
  bool isOfTypeName(const UTF8* name);
  bool isAssignableFrom(UserCommonClass* cl);
  
  /// subclassOf - If this class is a regular class, is it a subclass of the
  /// given class?
  ///
  bool subclassOf(UserCommonClass* cl);

  bool isArray() {
    return classDef->isArray();
  }
  bool isPrimitive() {
    return classDef->isPrimitive();
  }

  bool isInterface() {
    return classDef->isInterface();
  }

  bool isReady() {
    return status >= inClinit;
  }
  
  bool isResolved() {
    return status >= resolved;
  }

  uint8 getAccess() {
    return classDef->access;
  }

  const UTF8* getName() {
    return classDef->name;
  }

  void getDeclaredConstructors(std::vector<JavaMethod*>& res, bool publicOnly) {
    classDef->getDeclaredConstructors(res, publicOnly);
  }

  void getDeclaredFields(std::vector<JavaField*>& res, bool publicOnly) {
    classDef->getDeclaredFields(res, publicOnly);
  }

  void getDeclaredMethods(std::vector<JavaMethod*>& res, bool publicOnly) {
    classDef->getDeclaredMethods(res, publicOnly);
  }
  
  void initialiseClass(Jnjvm* vm);
  JavaObject* getClassDelegatee(Jnjvm* vm, JavaObject* pd = 0);

  void resolveClass();

  UserClass* getSuper() {
    return super;
  }

  std::vector<UserClass*>* getInterfaces() {
    return &interfaces;
  }

  CommonClass::field_map* getStaticFields() {
    return classDef->getStaticFields();
  }
  
  CommonClass::field_map* getVirtualFields() {
    return classDef->getVirtualFields();
  }
  
  CommonClass::method_map* getStaticMethods() {
    return classDef->getStaticMethods();
  }
  
  CommonClass::method_map* getVirtualMethods() {
    return classDef->getVirtualMethods();
  }
  
  void resolveStaticClass() {
    ((Class*)classDef)->resolveStaticClass();
  }


  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse,
                                    UserClass*& methodCl);

  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type,
                           bool isStatic, bool recurse, UserClass*& methodCl);

  JavaField* lookupField(const UTF8* name, const UTF8* type,
                         bool isStatic, bool recurse,
                         UserCommonClass*& fieldCl);
  
  JavaField* lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse,
                                  UserCommonClass*& fieldCl);
  
  
  uint32 getVirtualSize() {
    return virtualSize;
  }

  VirtualTable* getVirtualVT() {
    return virtualVT;
  }

  void setInterfaces(std::vector<UserClass*> Is) {
    interfaces = Is;
  }

  void setSuper(UserClass* S) {
    super = S;
  }

  bool instantiationOfArray(UserClassArray* cl);
  bool implements(UserCommonClass* cl);

  /// constructMethod - Add a new method in this class method map.
  ///
  JavaMethod* constructMethod(const UTF8* name, const UTF8* type,
                              uint32 access) {
    return classDef->constructMethod(name, type, access);
  }
  
  /// constructField - Add a new field in this class field map.
  ///
  JavaField* constructField(const UTF8* name, const UTF8* type,
                            uint32 access) {
    return classDef->constructField(name, type, access);
  }


  UserClass* lookupClassFromMethod(JavaMethod* meth);
  UserCommonClass* getUserClass(CommonClass* cl);
  
  /// lockVar - When multiple threads want to initialize a class,
  /// they must be synchronized so that it is only performed once
  /// for a given class.
  mvm::Lock* lockVar;

  /// condVar - Used to wake threads waiting on the initialization
  /// process of this class, done by another thread.
  mvm::Cond* condVar;

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

  const UTF8* getSuperUTF8(){
    return classDef->superUTF8;
  }

  std::vector<const UTF8*>* getInterfacesUTF8(){
    return &classDef->interfacesUTF8;
  }
  
  UserCommonClass();
};

class UserClass : public UserCommonClass {
public:
  static VirtualTable* VT;
  JavaObject* staticInstance;
  
  virtual void TRACER;

  UserClass(JnjvmClassLoader* JCL, const UTF8* name, ArrayUInt8* bytes);
  UserClass() {}

  JavaObject* doNew(Jnjvm* vm);
  
  std::vector<UserClass*> innerClasses;
  UserClass* outerClass;

  std::vector<UserClass*>* getInnerClasses() {
    return &innerClasses;
  }

  UserClass* getOuterClass() {
    return outerClass;
  }
  
  bool innerOuterResolved;

  void resolveInnerOuterClasses();
  
  void setInnerAccess(uint32 access) {
    ((Class*)classDef)->setInnerAccess(access);
  }

  JavaObject* getStaticInstance() {
    return staticInstance;
  }

  void setStaticInstance(JavaObject* obj) {
    staticInstance = obj;
  }
  
  UserConstantPool* getConstantPool() {
    return ctpInfo;
  }

  uint32 getStaticSize() {
    return ((Class*)classDef)->getStaticSize();
  }

  VirtualTable* getStaticVT() {
    return ((Class*)classDef)->getStaticVT();
  }
  
  /// loadParents - Loads and resolves the parents, i.e. super and interfarces,
  /// of the class.
  ///
  void loadParents();

  Attribut* lookupAttribut(const UTF8* Att) {
    return ((Class*)classDef)->lookupAttribut(Att);
  }

  ArrayUInt8* getBytes() {
    return ((Class*)classDef)->bytes;
  }
};

class UserClassArray : public UserCommonClass {
public:
  static VirtualTable* VT;
  UserCommonClass* _baseClass;
  AssessorDesc* _funcs;

  virtual void TRACER;
  UserClassArray(JnjvmClassLoader* JCL, const UTF8* name);
  UserClassArray() {}
  
  void resolveComponent();

  UserCommonClass* baseClass() {
    if (_baseClass == 0)
      resolveComponent();
    return _baseClass;
  }

  AssessorDesc* funcs();
};

class UserClassPrimitive : public UserCommonClass {
  static VirtualTable* VT;
public:
  
  virtual void TRACER;
  UserClassPrimitive(JnjvmClassLoader* JCL, const UTF8* name, uint32 nb);
};

class UserConstantPool : public mvm::Object {
public:
  
  static VirtualTable* VT;
  /// ctpRes - Objects resolved dynamically, e.g. UTF8s, classes, methods,
  /// fields, strings.
  ///
  void*  ctpRes[1]; 
  
  /// resolveMethod - Resolve the class and the signature of the method. May
  /// perform class loading. This function is called just in time, ie when
  /// the method call is actually made and not yet resolved.
  ///
  void resolveMethod(uint32 index, UserCommonClass*& cl,
                     const UTF8*& utf8, Signdef*& sign);
  
  /// resolveField - Resolve the class and signature of the field. May
  /// perform class loading. This function is called just in time, ie when
  /// the field is accessed and not yet resolved.
  ///
  void resolveField(uint32 index, UserCommonClass*& cl, const UTF8*& utf8,
                    Typedef*& sign);
  
  UserClass* getClass() {
    return (UserClass*)ctpRes[0];
  }

  JavaConstantPool* getSharedPool() {
    return ((Class*)(getClass()->classDef))->ctpInfo;
  }

  /// UTF8At - Get the UTF8 referenced from this string entry.
  ///
  const UTF8* UTF8AtForString(uint32 entry) {
    return getSharedPool()->UTF8AtForString(entry);
  }

  /// loadClass - Loads the class and returns it. This is called just in time, 
  /// ie when the class will be used and not yet resolved.
  ///
  UserCommonClass* loadClass(uint32 index);

  void* operator new(size_t sz, JavaAllocator* alloc, uint32 size);
  
  UserConstantPool(){}

  UserConstantPool(UserClass* cl) {
    ctpRes[0] = cl;
  }

  UserCommonClass* isClassLoaded(uint32 entry);

};

} // end namespace jnjvm

#endif // JNJVM_CLASS_ISOLATE_H
