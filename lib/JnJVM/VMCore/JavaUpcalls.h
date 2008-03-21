//===---------- JavaUpcalls.h - Upcalls to Java entities ------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_UPCALLS_H
#define JNJVM_JAVA_UPCALLS_H


#define UPCALL_CLASS(vm, name)                                            \
  vm->constructClass(vm->asciizConstructUTF8(name),                       \
                     CommonClass::jnjvmClassLoader)

#define UPCALL_FIELD(vm, cl, name, type, acc)                             \
  vm->constructField(UPCALL_CLASS(vm, cl), vm->asciizConstructUTF8(name), \
                     vm->asciizConstructUTF8(type), acc)

#define UPCALL_METHOD(vm, cl, name, type, acc)                             \
  vm->constructMethod(UPCALL_CLASS(vm, cl), vm->asciizConstructUTF8(name), \
                      vm->asciizConstructUTF8(type), acc)

#define UPCALL_ARRAY_CLASS(vm, name, depth)                                \
  vm->constructArray(                                                      \
    AssessorDesc::constructArrayName(vm, 0, depth,                         \
                                     vm->asciizConstructUTF8(name)),       \
    CommonClass::jnjvmClassLoader)


namespace jnjvm {

class Jnjvm;
class JavaField;
class JavaMethod;
class Class;

class ClasspathThread {
public:
  static void initialise(Jnjvm* vm);
  
  static Class* newThread;
  static Class* newVMThread;
  static JavaField* assocThread;
  static JavaField* vmdata;
  static JavaMethod* finaliseCreateInitialThread;
  static JavaMethod* initVMThread;
  static JavaMethod* groupAddThread;
  static JavaField* name;
  static JavaField* priority;
  static JavaField* daemon;
  static JavaField* group;
  static JavaField* running;
  static JavaField* rootGroup;
  static JavaField* vmThread;
  static JavaMethod* uncaughtException;
  
  static void createInitialThread(Jnjvm* vm, JavaObject* th);
  static void mapInitialThread(Jnjvm* vm);
};

class Classpath {
public: 
  static JavaMethod* getSystemClassLoader;
  static JavaMethod* setContextClassLoader;
  static Class* newString;
  static Class* newClass;
  static Class* newThrowable;
  static Class* newException;
  static JavaMethod* initClass;
  static JavaMethod* initClassWithProtectionDomain;
  static JavaField* vmdataClass;
  static JavaMethod* setProperty;
  static JavaMethod* initString;
  static JavaMethod* getCallingClassLoader;
  static JavaMethod* initConstructor;
  static ClassArray* constructorArrayClass;
  static Class*      newConstructor;
  static JavaField*  constructorSlot;
  static JavaMethod* initMethod;
  static JavaMethod* initField;
  static ClassArray* methodArrayClass;
  static ClassArray* fieldArrayClass;
  static Class*      newMethod;
  static Class*      newField;
  static JavaField*  methodSlot;
  static JavaField*  fieldSlot;
  static ClassArray* classArrayClass;
  static JavaMethod* loadInClassLoader;
  static JavaMethod* initVMThrowable;
  static JavaField*  vmDataVMThrowable;
  static Class*      newVMThrowable;
  static JavaField*  bufferAddress;
  static JavaField*  dataPointer32;
  static JavaField*  vmdataClassLoader;

  static JavaField* boolValue;
  static JavaField* byteValue;
  static JavaField* shortValue;
  static JavaField* charValue;
  static JavaField* intValue;
  static JavaField* longValue;
  static JavaField* floatValue;
  static JavaField* doubleValue;

  static Class* newStackTraceElement;
  static JavaMethod* initStackTraceElement;

  static void initialiseClasspath(Jnjvm* vm);
  
  static Class* voidClass;
  static Class* boolClass;
  static Class* byteClass;
  static Class* shortClass;
  static Class* charClass;
  static Class* intClass;
  static Class* floatClass;
  static Class* doubleClass;
  static Class* longClass;
  
  static Class* vmStackWalker;
};


} // end namespace jnjvm

#endif
