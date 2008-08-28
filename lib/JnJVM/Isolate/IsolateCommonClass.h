//===---- IsolateCommonClass.h - User visible classes with isolates -------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef ISOLATE_COMMON_CLASS_H
#define ISOLATE_COMMON_CLASS_H

#include "mvm/Object.h"

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
  CommonClass** display;
  
  /// depth - The depth of this class in its class hierarchy. 
  /// display[depth] contains the class. Array classes do not need it.
  ///
  uint32 depth;

  /// status - The loading/resolve/initialization state of the class.
  ///
  JavaState status;

//===----------------------------------------------------------------------===//
//
// New fields can be added from now, or reordered.
//
//===----------------------------------------------------------------------===// 

  JnjvmClassLoader* classLoader;
  JavaObject* delegatee;
  CommonClass* classDef;

  virtual void TRACER;

  bool isOfTypeName(const UTF8* name);
  bool isAssignableFrom(UserCommonClass* cl);
  
  /// subclassOf - If this class is a regular class, is it a subclass of the
  /// given class?
  ///
  bool subclassOf(UserCommonClass* cl);

  bool isArray();
  bool isPrimitive();
  bool isInterface();
  bool isReady();
  uint8 getAccess();
  const UTF8* getName();

  void getDeclaredConstructors(std::vector<JavaMethod*>& res, bool publicOnly);
  void getDeclaredFields(std::vector<JavaField*>& res, bool publicOnly);
  void getDeclaredMethods(std::vector<JavaMethod*>& res, bool publicOnly);
  
  void initialiseClass(Jnjvm* vm);
  JavaObject* getClassDelegatee(Jnjvm* vm, JavaObject* pd = 0);

  void resolveClass();
  UserClass* getSuper();

  std::vector<UserClass*>* getInterfaces();
  CommonClass::field_map* getStaticFields();
  void resolveStaticClass();


  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse);
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type,
                           bool isStatic, bool recurse);
  JavaField* lookupField(const UTF8* name, const UTF8* type,
                         bool isStatic, bool recurse,
                         UserCommonClass*& fieldCl);
  
  uint64 getVirtualSize();
  VirtualTable* getVirtualVT();

  void setInterfaces(std::vector<UserClass*> Is);
  void setSuper(UserClass* S);

  bool instantiationOfArray(UserClassArray* cl);
  bool implements(UserCommonClass* cl);

  /// constructMethod - Add a new method in this class method map.
  ///
  JavaMethod* constructMethod(const UTF8* name, const UTF8* type,
                              uint32 access);
  
  /// constructField - Add a new field in this class field map.
  ///
  JavaField* constructField(const UTF8* name, const UTF8* type,
                            uint32 access);

  UserConstantPool* getCtpCache();

  UserClass* lookupClassFromMethod(JavaMethod* meth);
  
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

};

class UserClass : public UserCommonClass {
public:
  static VirtualTable* VT;
  JavaObject* staticInstance;
  
  virtual void TRACER;

  UserClass(JnjvmClassLoader* JCL, const UTF8* name, ArrayUInt8* bytes);
  
  JavaObject* doNew(Jnjvm* vm);
  
  std::vector<UserClass*>* getInnerClasses();
  UserClass* getOuterClass();
  void resolveInnerOuterClasses();
  JavaObject* getStaticInstance();
  void setStaticInstance(JavaObject* obj);
  UserConstantPool* getConstantPool();

  void setStaticSize(uint64 size);
  void setStaticVT(VirtualTable* VT);
  
  uint64 getStaticSize();
  VirtualTable* getStaticVT();
};

class UserClassArray : public UserCommonClass {
public:
  static VirtualTable* VT;
  UserCommonClass* _baseClass;
  
  virtual void TRACER;
  UserClassArray(JnjvmClassLoader* JCL, const UTF8* name);

  UserCommonClass* baseClass();

  AssessorDesc* funcs();
};

class UserClassPrimitive : public UserCommonClass {
  static VirtualTable* VT;
public:
  
  virtual void TRACER;
  UserClassPrimitive(JnjvmClassLoader* JCL, const UTF8* name, uint32 nb);
};

class UserConstantPool {
public:
  
  /// ctpRes - Objects resolved dynamically, e.g. UTF8s, classes, methods,
  /// fields, strings.
  ///
  void**  ctpRes; 
  
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

  /// UTF8At - Get the UTF8 referenced from this string entry.
  ///
  const UTF8* UTF8AtForString(uint32 entry);

  /// loadClass - Loads the class and returns it. This is called just in time, 
  /// ie when the class will be used and not yet resolved.
  ///
  UserCommonClass* loadClass(uint32 index);
};

} // end namespace jnjvm

#endif // JNJVM_CLASS_ISOLATE_H
