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
class JavaField;
class JavaMethod;
class JavaObject;
class Jnjvm;
class JnjvmClassLoader;
class UserClass;
class UTF8;

class UserCommonClass : public mvm::Object {
public:
  CommonClass* classDef;
  JnjvmClassLoader* classLoader;
  JavaObject* delegatee;
  uint8 status;

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

  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse);
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type,
                           bool isStatic, bool recurse);
  
  uint64 getVirtualSize();
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
  UserClassPrimitive(JnjvmClassLoader* JCL, const UTF8* name);
};

} // end namespace jnjvm

#endif // JNJVM_CLASS_ISOLATE_H
