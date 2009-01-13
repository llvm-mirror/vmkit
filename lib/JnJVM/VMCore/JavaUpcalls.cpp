//===-------- JavaUpcalls.cpp - Upcalls to Java entities ------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

#define COMPILE_METHODS(cl) \
  for (CommonClass::method_iterator i = cl->virtualMethods.begin(), \
            e = cl->virtualMethods.end(); i!= e; ++i) { \
    i->second->compiledPtr(); \
  } \
  \
  for (CommonClass::method_iterator i = cl->staticMethods.begin(), \
            e = cl->staticMethods.end(); i!= e; ++i) { \
    i->second->compiledPtr(); \
  }


using namespace jnjvm;

#ifndef ISOLATE_SHARING
Class*      Classpath::newThread;
Class*      Classpath::newVMThread;
JavaField*  Classpath::assocThread;
JavaField*  Classpath::vmdataVMThread;
JavaMethod* Classpath::finaliseCreateInitialThread;
JavaMethod* Classpath::initVMThread;
JavaMethod* Classpath::groupAddThread;
JavaField*  Classpath::name;
JavaField*  Classpath::priority;
JavaField*  Classpath::daemon;
JavaField*  Classpath::group;
JavaField*  Classpath::running;
Class*      Classpath::threadGroup;
JavaField*  Classpath::rootGroup;
JavaField*  Classpath::vmThread;
JavaMethod* Classpath::uncaughtException;
Class*      Classpath::inheritableThreadLocal;

JavaMethod* Classpath::runVMThread;
JavaMethod* Classpath::setContextClassLoader;
JavaMethod* Classpath::getSystemClassLoader;
Class*      Classpath::newString;
Class*      Classpath::newClass;
Class*      Classpath::newThrowable;
Class*      Classpath::newException;
JavaMethod* Classpath::initClass;
JavaMethod* Classpath::initClassWithProtectionDomain;
JavaField*  Classpath::vmdataClass;
JavaMethod* Classpath::setProperty;
JavaMethod* Classpath::initString;
JavaMethod* Classpath::getCallingClassLoader;
JavaMethod* Classpath::initConstructor;
Class*      Classpath::newConstructor;
ClassArray* Classpath::constructorArrayClass;
ClassArray* Classpath::constructorArrayAnnotation;
JavaField*  Classpath::constructorSlot;
JavaMethod* Classpath::initMethod;
JavaMethod* Classpath::initField;
Class*      Classpath::newField;
Class*      Classpath::newMethod;
ClassArray* Classpath::methodArrayClass;
ClassArray* Classpath::fieldArrayClass;
JavaField*  Classpath::methodSlot;
JavaField*  Classpath::fieldSlot;
ClassArray* Classpath::classArrayClass;
JavaMethod* Classpath::loadInClassLoader;
JavaMethod* Classpath::initVMThrowable;
JavaField*  Classpath::vmDataVMThrowable;
Class*      Classpath::newVMThrowable;
JavaField*  Classpath::bufferAddress;
JavaField*  Classpath::dataPointer32;
JavaField*  Classpath::vmdataClassLoader;
Class*      Classpath::newClassLoader;


JavaField*  Classpath::boolValue;
JavaField*  Classpath::byteValue;
JavaField*  Classpath::shortValue;
JavaField*  Classpath::charValue;
JavaField*  Classpath::intValue;
JavaField*  Classpath::longValue;
JavaField*  Classpath::floatValue;
JavaField*  Classpath::doubleValue;

Class*      Classpath::newStackTraceElement;
ClassArray* Classpath::stackTraceArray;
JavaMethod* Classpath::initStackTraceElement;

Class* Classpath::voidClass;
Class* Classpath::boolClass;
Class* Classpath::byteClass;
Class* Classpath::shortClass;
Class* Classpath::charClass;
Class* Classpath::intClass;
Class* Classpath::floatClass;
Class* Classpath::doubleClass;
Class* Classpath::longClass;

Class* Classpath::vmStackWalker;

Class* Classpath::InvocationTargetException;
Class* Classpath::ArrayStoreException;
Class* Classpath::ClassCastException;
Class* Classpath::IllegalMonitorStateException;
Class* Classpath::IllegalArgumentException;
Class* Classpath::InterruptedException;
Class* Classpath::IndexOutOfBoundsException;
Class* Classpath::ArrayIndexOutOfBoundsException;
Class* Classpath::NegativeArraySizeException;
Class* Classpath::NullPointerException;
Class* Classpath::SecurityException;
Class* Classpath::ClassFormatError;
Class* Classpath::ClassCircularityError;
Class* Classpath::NoClassDefFoundError;
Class* Classpath::UnsupportedClassVersionError;
Class* Classpath::NoSuchFieldError;
Class* Classpath::NoSuchMethodError;
Class* Classpath::InstantiationError;
Class* Classpath::IllegalAccessError;
Class* Classpath::IllegalAccessException;
Class* Classpath::VerifyError;
Class* Classpath::ExceptionInInitializerError;
Class* Classpath::LinkageError;
Class* Classpath::AbstractMethodError;
Class* Classpath::UnsatisfiedLinkError;
Class* Classpath::InternalError;
Class* Classpath::OutOfMemoryError;
Class* Classpath::StackOverflowError;
Class* Classpath::UnknownError;
Class* Classpath::ClassNotFoundException;

JavaMethod* Classpath::InitInvocationTargetException;
JavaMethod* Classpath::InitArrayStoreException;
JavaMethod* Classpath::InitClassCastException;
JavaMethod* Classpath::InitIllegalMonitorStateException;
JavaMethod* Classpath::InitIllegalArgumentException;
JavaMethod* Classpath::InitInterruptedException;
JavaMethod* Classpath::InitIndexOutOfBoundsException;
JavaMethod* Classpath::InitArrayIndexOutOfBoundsException;
JavaMethod* Classpath::InitNegativeArraySizeException;
JavaMethod* Classpath::InitNullPointerException;
JavaMethod* Classpath::InitSecurityException;
JavaMethod* Classpath::InitClassFormatError;
JavaMethod* Classpath::InitClassCircularityError;
JavaMethod* Classpath::InitNoClassDefFoundError;
JavaMethod* Classpath::InitUnsupportedClassVersionError;
JavaMethod* Classpath::InitNoSuchFieldError;
JavaMethod* Classpath::InitNoSuchMethodError;
JavaMethod* Classpath::InitInstantiationError;
JavaMethod* Classpath::InitIllegalAccessError;
JavaMethod* Classpath::InitIllegalAccessException;
JavaMethod* Classpath::InitVerifyError;
JavaMethod* Classpath::InitExceptionInInitializerError;
JavaMethod* Classpath::InitLinkageError;
JavaMethod* Classpath::InitAbstractMethodError;
JavaMethod* Classpath::InitUnsatisfiedLinkError;
JavaMethod* Classpath::InitInternalError;
JavaMethod* Classpath::InitOutOfMemoryError;
JavaMethod* Classpath::InitStackOverflowError;
JavaMethod* Classpath::InitUnknownError;
JavaMethod* Classpath::InitClassNotFoundException;

JavaMethod* Classpath::ErrorWithExcpNoClassDefFoundError;
JavaMethod* Classpath::ErrorWithExcpExceptionInInitializerError;
JavaMethod* Classpath::ErrorWithExcpInvocationTargetException;

ClassArray* Classpath::ArrayOfByte;
ClassArray* Classpath::ArrayOfChar;
ClassArray* Classpath::ArrayOfString;
ClassArray* Classpath::ArrayOfInt;
ClassArray* Classpath::ArrayOfShort;
ClassArray* Classpath::ArrayOfBool;
ClassArray* Classpath::ArrayOfLong;
ClassArray* Classpath::ArrayOfFloat;
ClassArray* Classpath::ArrayOfDouble;
ClassArray* Classpath::ArrayOfObject;

ClassPrimitive* Classpath::OfByte;
ClassPrimitive* Classpath::OfChar;
ClassPrimitive* Classpath::OfInt;
ClassPrimitive* Classpath::OfShort;
ClassPrimitive* Classpath::OfBool;
ClassPrimitive* Classpath::OfLong;
ClassPrimitive* Classpath::OfFloat;
ClassPrimitive* Classpath::OfDouble;
ClassPrimitive* Classpath::OfVoid;

JavaField* Classpath::methodClass;
JavaField* Classpath::fieldClass;
JavaField* Classpath::constructorClass;

#endif

void Classpath::createInitialThread(Jnjvm* vm, JavaObject* th) {
  JnjvmClassLoader* JCL = vm->bootstrapLoader;
  JCL->loadName(newVMThread->getName(), true, true);
  newVMThread->initialiseClass(vm);

  JavaObject* vmth = newVMThread->doNew(vm);
  name->setObjectField(th, (JavaObject*)vm->asciizToStr("main"));
  priority->setInt32Field(th, (uint32)1);
  daemon->setInt8Field(th, (uint32)0);
  vmThread->setObjectField(th, vmth);
  assocThread->setObjectField(vmth, th);
  running->setInt8Field(vmth, (uint32)1);
  
  JCL->loadName(threadGroup->getName(), true, true);
  threadGroup->initialiseClass(vm);
  void* Stat = threadGroup->getStaticInstance();
  JavaObject* RG = rootGroup->getObjectField(Stat);
  group->setObjectField(th, RG);
  groupAddThread->invokeIntSpecial(vm, threadGroup, RG, th);
}

void Classpath::mapInitialThread(Jnjvm* vm) {
  JnjvmClassLoader* JCL = vm->bootstrapLoader;
  JCL->loadName(newThread->getName(), true, true);
  newThread->initialiseClass(vm);
  JavaObject* th = newThread->doNew(vm);
  createInitialThread(vm, th);
  JavaThread* myth = JavaThread::get();
  myth->javaThread = th;
  JavaObject* vmth = vmThread->getObjectField(th);
  vmdataVMThread->setObjectField(vmth, (JavaObject*)myth);
  finaliseCreateInitialThread->invokeIntStatic(vm, inheritableThreadLocal, th);
}

extern "C" JavaString* nativeInternString(JavaString* obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  const UTF8* utf8 = obj->strToUTF8(vm);
  return vm->UTF8ToStr(utf8);
}

extern "C" uint8 nativeIsArray(JavaObject* klass) {
  UserCommonClass* cl = ((JavaObjectClass*)klass)->getClass();  
  return (uint8)cl->isArray();
}

extern "C" JavaObject* nativeGetCallingClass() {
  
  JavaObject* res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  UserClass* cl = th->getCallingClass(1);
  if (cl) res = cl->getClassDelegatee(th->getJVM());
  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* nativeGetCallingClassLoader() {
  
  JavaObject *res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)
  JavaThread* th = JavaThread::get();
  UserClass* cl = th->getCallingClass(1);
  res = cl->classLoader->getJavaClassLoader();  
  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* nativeFirstNonNullClassLoader() {
  JavaObject *res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)
  JavaThread* th = JavaThread::get();
  res = th->getNonNullClassLoader();
  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* nativeGetCallerClass(uint32 index) {
  
  JavaObject *res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* cl = th->getCallingClassLevel(index - 1);
  if (cl) res = cl->getClassDelegatee(vm);
  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* nativeGetAnnotation(JavaObject* obj) {
  return 0;
}

extern "C" JavaObject* nativeGetDeclaredAnnotations() {
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassArray* array = vm->upcalls->constructorArrayAnnotation;
  return array->doNew(0, vm);
}

extern "C" void nativePropertiesPostInit(JavaObject* prop);


void Classpath::initialiseClasspath(JnjvmClassLoader* loader) {

  newClassLoader = 
    UPCALL_CLASS(loader, "java/lang/ClassLoader");
  
  getSystemClassLoader =
    UPCALL_METHOD(loader, "java/lang/ClassLoader", "getSystemClassLoader",
                  "()Ljava/lang/ClassLoader;", ACC_STATIC);

  setContextClassLoader =
    UPCALL_METHOD(loader, "java/lang/Thread", "setContextClassLoader",
                  "(Ljava/lang/ClassLoader;)V", ACC_VIRTUAL);

  newString = 
    UPCALL_CLASS(loader, "java/lang/String");
  
  newClass =
    UPCALL_CLASS(loader, "java/lang/Class");
  
  newThrowable =
    UPCALL_CLASS(loader, "java/lang/Throwable");
  
  newException =
    UPCALL_CLASS(loader, "java/lang/Exception");
  
  initClass =
    UPCALL_METHOD(loader, "java/lang/Class", "<init>", "(Ljava/lang/Object;)V",
                  ACC_VIRTUAL);

  initClassWithProtectionDomain =
    UPCALL_METHOD(loader, "java/lang/Class", "<init>",
                  "(Ljava/lang/Object;Ljava/security/ProtectionDomain;)V",
                  ACC_VIRTUAL);

  vmdataClass =
    UPCALL_FIELD(loader, "java/lang/Class", "vmdata", "Ljava/lang/Object;",
                 ACC_VIRTUAL);
  
  setProperty = 
    UPCALL_METHOD(loader, "java/util/Properties", "setProperty",
                  "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;",
                  ACC_VIRTUAL);

  initString =
    UPCALL_METHOD(loader, "java/lang/String", "<init>", "([CIIZ)V", ACC_VIRTUAL);
  
  initConstructor =
    UPCALL_METHOD(loader, "java/lang/reflect/Constructor", "<init>",
                  "(Ljava/lang/Class;I)V", ACC_VIRTUAL);

  newConstructor =
    UPCALL_CLASS(loader, "java/lang/reflect/Constructor");

  constructorArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/reflect/Constructor", 1);
  
  constructorArrayAnnotation =
    UPCALL_ARRAY_CLASS(loader, "java/lang/annotation/Annotation", 1);

  constructorSlot =
    UPCALL_FIELD(loader, "java/lang/reflect/Constructor", "slot", "I", ACC_VIRTUAL);
  
  initMethod =
    UPCALL_METHOD(loader, "java/lang/reflect/Method", "<init>",
                  "(Ljava/lang/Class;Ljava/lang/String;I)V", ACC_VIRTUAL);

  newMethod =
    UPCALL_CLASS(loader, "java/lang/reflect/Method");

  methodArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/reflect/Method", 1);

  methodSlot =
    UPCALL_FIELD(loader, "java/lang/reflect/Method", "slot", "I", ACC_VIRTUAL);
  
  initField =
    UPCALL_METHOD(loader, "java/lang/reflect/Field", "<init>",
                  "(Ljava/lang/Class;Ljava/lang/String;I)V", ACC_VIRTUAL);

  newField =
    UPCALL_CLASS(loader, "java/lang/reflect/Field");

  fieldArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/reflect/Field", 1);
  
  fieldSlot =
    UPCALL_FIELD(loader, "java/lang/reflect/Field", "slot", "I", ACC_VIRTUAL);
  
  
  classArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/Class", 1);
  
  newVMThrowable =
    UPCALL_CLASS(loader, "java/lang/VMThrowable");
  
  initVMThrowable =
    UPCALL_METHOD(loader, "java/lang/VMThrowable", "<init>", "()V", ACC_VIRTUAL);

  vmDataVMThrowable =
    UPCALL_FIELD(loader, "java/lang/VMThrowable", "vmdata", "Ljava/lang/Object;",
                 ACC_VIRTUAL);

  bufferAddress =
    UPCALL_FIELD(loader, "java/nio/Buffer", "address", "Lgnu/classpath/Pointer;",
                 ACC_VIRTUAL);

  dataPointer32 =
    UPCALL_FIELD(loader, "gnu/classpath/Pointer32", "data", "I", ACC_VIRTUAL);

  vmdataClassLoader =
    UPCALL_FIELD(loader, "java/lang/ClassLoader", "vmdata", "Ljava/lang/Object;",
                 ACC_VIRTUAL);
  
  newStackTraceElement =
    UPCALL_CLASS(loader, "java/lang/StackTraceElement");
  
  stackTraceArray =
    UPCALL_ARRAY_CLASS(loader, "java/lang/StackTraceElement", 1);

  initStackTraceElement =
    UPCALL_METHOD(loader,  "java/lang/StackTraceElement", "<init>",
                  "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V",
                  ACC_VIRTUAL);

  boolValue =
    UPCALL_FIELD(loader, "java/lang/Boolean", "value", "Z", ACC_VIRTUAL);
  
  byteValue =
    UPCALL_FIELD(loader, "java/lang/Byte", "value", "B", ACC_VIRTUAL);

  shortValue =
    UPCALL_FIELD(loader, "java/lang/Short", "value", "S", ACC_VIRTUAL);

  charValue =
    UPCALL_FIELD(loader, "java/lang/Character", "value", "C", ACC_VIRTUAL);

  intValue =
    UPCALL_FIELD(loader, "java/lang/Integer", "value", "I", ACC_VIRTUAL);

  longValue =
    UPCALL_FIELD(loader, "java/lang/Long", "value", "J", ACC_VIRTUAL);

  floatValue =
    UPCALL_FIELD(loader, "java/lang/Float", "value", "F", ACC_VIRTUAL);

  doubleValue =
    UPCALL_FIELD(loader, "java/lang/Double", "value", "D", ACC_VIRTUAL);

  Classpath::voidClass =
    UPCALL_CLASS(loader, "java/lang/Void");
  
  Classpath::boolClass =
    UPCALL_CLASS(loader, "java/lang/Boolean");

  Classpath::byteClass =
    UPCALL_CLASS(loader, "java/lang/Byte");

  Classpath::shortClass =
    UPCALL_CLASS(loader, "java/lang/Short");

  Classpath::charClass =
    UPCALL_CLASS(loader, "java/lang/Character"); 

  Classpath::intClass =
    UPCALL_CLASS(loader, "java/lang/Integer");

  Classpath::floatClass =
    UPCALL_CLASS(loader, "java/lang/Float");

  Classpath::doubleClass =
    UPCALL_CLASS(loader, "java/lang/Double");

  Classpath::longClass =
    UPCALL_CLASS(loader, "java/lang/Long");

  vmStackWalker =
    UPCALL_CLASS(loader, "gnu/classpath/VMStackWalker");

  loadInClassLoader =
    UPCALL_METHOD(loader, "java/lang/ClassLoader", "loadClass",
                  "(Ljava/lang/String;Z)Ljava/lang/Class;", ACC_VIRTUAL);

  JavaMethod* internString =
    UPCALL_METHOD(loader, "java/lang/VMString", "intern",
                  "(Ljava/lang/String;)Ljava/lang/String;", ACC_STATIC); 
  internString->setCompiledPtr((void*)(intptr_t)nativeInternString,
                               "nativeInternString");
  
  JavaMethod* isArray =
    UPCALL_METHOD(loader, "java/lang/Class", "isArray", "()Z", ACC_VIRTUAL);
  isArray->setCompiledPtr((void*)(intptr_t)nativeIsArray, "nativeIsArray");


  UPCALL_REFLECT_CLASS_EXCEPTION(loader, InvocationTargetException);
  UPCALL_CLASS_EXCEPTION(loader, ArrayStoreException);
  UPCALL_CLASS_EXCEPTION(loader, ClassCastException);
  UPCALL_CLASS_EXCEPTION(loader, IllegalMonitorStateException);
  UPCALL_CLASS_EXCEPTION(loader, IllegalArgumentException);
  UPCALL_CLASS_EXCEPTION(loader, InterruptedException);
  UPCALL_CLASS_EXCEPTION(loader, IndexOutOfBoundsException);
  UPCALL_CLASS_EXCEPTION(loader, ArrayIndexOutOfBoundsException);
  UPCALL_CLASS_EXCEPTION(loader, NegativeArraySizeException);
  UPCALL_CLASS_EXCEPTION(loader, NullPointerException);
  UPCALL_CLASS_EXCEPTION(loader, SecurityException);
  UPCALL_CLASS_EXCEPTION(loader, ClassFormatError);
  UPCALL_CLASS_EXCEPTION(loader, ClassCircularityError);
  UPCALL_CLASS_EXCEPTION(loader, NoClassDefFoundError);
  UPCALL_CLASS_EXCEPTION(loader, UnsupportedClassVersionError);
  UPCALL_CLASS_EXCEPTION(loader, NoSuchFieldError);
  UPCALL_CLASS_EXCEPTION(loader, NoSuchMethodError);
  UPCALL_CLASS_EXCEPTION(loader, InstantiationError);
  UPCALL_CLASS_EXCEPTION(loader, IllegalAccessError);
  UPCALL_CLASS_EXCEPTION(loader, IllegalAccessException);
  UPCALL_CLASS_EXCEPTION(loader, VerifyError);
  UPCALL_CLASS_EXCEPTION(loader, ExceptionInInitializerError);
  UPCALL_CLASS_EXCEPTION(loader, LinkageError);
  UPCALL_CLASS_EXCEPTION(loader, AbstractMethodError);
  UPCALL_CLASS_EXCEPTION(loader, UnsatisfiedLinkError);
  UPCALL_CLASS_EXCEPTION(loader, InternalError);
  UPCALL_CLASS_EXCEPTION(loader, OutOfMemoryError);
  UPCALL_CLASS_EXCEPTION(loader, StackOverflowError);
  UPCALL_CLASS_EXCEPTION(loader, UnknownError);
  UPCALL_CLASS_EXCEPTION(loader, ClassNotFoundException);
  
  UPCALL_METHOD_EXCEPTION(loader, InvocationTargetException);
  UPCALL_METHOD_EXCEPTION(loader, ArrayStoreException);
  UPCALL_METHOD_EXCEPTION(loader, ClassCastException);
  UPCALL_METHOD_EXCEPTION(loader, IllegalMonitorStateException);
  UPCALL_METHOD_EXCEPTION(loader, IllegalArgumentException);
  UPCALL_METHOD_EXCEPTION(loader, InterruptedException);
  UPCALL_METHOD_EXCEPTION(loader, IndexOutOfBoundsException);
  UPCALL_METHOD_EXCEPTION(loader, ArrayIndexOutOfBoundsException);
  UPCALL_METHOD_EXCEPTION(loader, NegativeArraySizeException);
  UPCALL_METHOD_EXCEPTION(loader, NullPointerException);
  UPCALL_METHOD_EXCEPTION(loader, SecurityException);
  UPCALL_METHOD_EXCEPTION(loader, ClassFormatError);
  UPCALL_METHOD_EXCEPTION(loader, ClassCircularityError);
  UPCALL_METHOD_EXCEPTION(loader, NoClassDefFoundError);
  UPCALL_METHOD_EXCEPTION(loader, UnsupportedClassVersionError);
  UPCALL_METHOD_EXCEPTION(loader, NoSuchFieldError);
  UPCALL_METHOD_EXCEPTION(loader, NoSuchMethodError);
  UPCALL_METHOD_EXCEPTION(loader, InstantiationError);
  UPCALL_METHOD_EXCEPTION(loader, IllegalAccessError);
  UPCALL_METHOD_EXCEPTION(loader, IllegalAccessException);
  UPCALL_METHOD_EXCEPTION(loader, VerifyError);
  UPCALL_METHOD_EXCEPTION(loader, ExceptionInInitializerError);
  UPCALL_METHOD_EXCEPTION(loader, LinkageError);
  UPCALL_METHOD_EXCEPTION(loader, AbstractMethodError);
  UPCALL_METHOD_EXCEPTION(loader, UnsatisfiedLinkError);
  UPCALL_METHOD_EXCEPTION(loader, InternalError);
  UPCALL_METHOD_EXCEPTION(loader, OutOfMemoryError);
  UPCALL_METHOD_EXCEPTION(loader, StackOverflowError);
  UPCALL_METHOD_EXCEPTION(loader, UnknownError);
  UPCALL_METHOD_EXCEPTION(loader, ClassNotFoundException);
  
  UPCALL_METHOD_WITH_EXCEPTION(loader, NoClassDefFoundError);
  UPCALL_METHOD_WITH_EXCEPTION(loader, ExceptionInInitializerError);
  UPCALL_METHOD_WITH_EXCEPTION(loader, InvocationTargetException);


  newThread = 
    UPCALL_CLASS(loader, "java/lang/Thread");
  
  newVMThread = 
    UPCALL_CLASS(loader, "java/lang/VMThread");
  
  assocThread = 
    UPCALL_FIELD(loader, "java/lang/VMThread", "thread", "Ljava/lang/Thread;",
                 ACC_VIRTUAL);
  
  vmdataVMThread = 
    UPCALL_FIELD(loader, "java/lang/VMThread", "vmdata", "Ljava/lang/Object;",
                 ACC_VIRTUAL);
  
  inheritableThreadLocal = 
    UPCALL_CLASS(loader, "java/lang/InheritableThreadLocal");

  finaliseCreateInitialThread = 
    UPCALL_METHOD(loader, "java/lang/InheritableThreadLocal", "newChildThread",
                  "(Ljava/lang/Thread;)V", ACC_STATIC);
  
  initVMThread = 
    UPCALL_METHOD(loader, "java/lang/VMThread", "<init>",
                  "(Ljava/lang/Thread;)V", ACC_VIRTUAL);
  
  runVMThread = 
    UPCALL_METHOD(loader, "java/lang/VMThread", "run", "()V", ACC_VIRTUAL);


  groupAddThread = 
    UPCALL_METHOD(loader, "java/lang/ThreadGroup", "addThread",
                  "(Ljava/lang/Thread;)V", ACC_VIRTUAL);
  
  name = 
    UPCALL_FIELD(loader, "java/lang/Thread", "name", "Ljava/lang/String;",
                 ACC_VIRTUAL);
  
  priority = 
    UPCALL_FIELD(loader,  "java/lang/Thread", "priority", "I", ACC_VIRTUAL);

  daemon = 
    UPCALL_FIELD(loader, "java/lang/Thread", "daemon", "Z", ACC_VIRTUAL);

  group =
    UPCALL_FIELD(loader, "java/lang/Thread", "group",
                 "Ljava/lang/ThreadGroup;", ACC_VIRTUAL);
  
  running = 
    UPCALL_FIELD(loader, "java/lang/VMThread", "running", "Z", ACC_VIRTUAL);
  
  threadGroup = 
    UPCALL_CLASS(loader, "java/lang/ThreadGroup");
  
  rootGroup =
    UPCALL_FIELD(loader, "java/lang/ThreadGroup", "root",
                 "Ljava/lang/ThreadGroup;", ACC_STATIC);

  vmThread = 
    UPCALL_FIELD(loader, "java/lang/Thread", "vmThread",
                 "Ljava/lang/VMThread;", ACC_VIRTUAL);
  
  uncaughtException = 
    UPCALL_METHOD(loader, "java/lang/ThreadGroup",  "uncaughtException",
                  "(Ljava/lang/Thread;Ljava/lang/Throwable;)V", ACC_VIRTUAL);

  
  methodClass =
    UPCALL_FIELD(loader, "java/lang/reflect/Method", "declaringClass",
                 "Ljava/lang/Class;", ACC_VIRTUAL);
  
  fieldClass =
    UPCALL_FIELD(loader, "java/lang/reflect/Field", "declaringClass",
                 "Ljava/lang/Class;", ACC_VIRTUAL);
  
  constructorClass =
    UPCALL_FIELD(loader, "java/lang/reflect/Constructor", "clazz",
                 "Ljava/lang/Class;", ACC_VIRTUAL);

  loader->loadName(loader->asciizConstructUTF8("java/lang/String"), 
                                       true, false);

  loader->loadName(loader->asciizConstructUTF8("java/lang/Object"), 
                                       true, false);
  
  // Don't compile methods here, we still don't know where to allocate Java
  // strings.

  JavaMethod* getCallingClass =
    UPCALL_METHOD(loader, "gnu/classpath/VMStackWalker", "getCallingClass",
                  "()Ljava/lang/Class;", ACC_STATIC);
  getCallingClass->setCompiledPtr((void*)(intptr_t)nativeGetCallingClass,
                                  "nativeGetCallingClass");
  
  JavaMethod* getCallingClassLoader =
    UPCALL_METHOD(loader, "gnu/classpath/VMStackWalker", "getCallingClassLoader",
                  "()Ljava/lang/ClassLoader;", ACC_STATIC);
  getCallingClassLoader->setCompiledPtr((void*)(intptr_t)
                                        nativeGetCallingClassLoader,
                                        "nativeGetCallingClassLoader");
  
  JavaMethod* firstNonNullClassLoader =
    UPCALL_METHOD(loader, "gnu/classpath/VMStackWalker", "firstNonNullClassLoader",
                  "()Ljava/lang/ClassLoader;", ACC_STATIC);
  firstNonNullClassLoader->setCompiledPtr((void*)(intptr_t)
                                          nativeFirstNonNullClassLoader,
                                          "nativeFirstNonNullClassLoader");
  
  JavaMethod* getCallerClass =
    UPCALL_METHOD(loader, "sun/reflect/Reflection", "getCallerClass",
                  "(I)Ljava/lang/Class;", ACC_STATIC);
  getCallerClass->setCompiledPtr((void*)(intptr_t)nativeGetCallerClass,
                                 "nativeGetCallerClass");
  
  JavaMethod* postProperties =
    UPCALL_METHOD(loader, "gnu/classpath/VMSystemProperties", "postInit",
                  "(Ljava/util/Properties;)V", ACC_STATIC);
  postProperties->setCompiledPtr((void*)(intptr_t)nativePropertiesPostInit,
                                 "nativePropertiesPostInit");

  // Also implement these twos, implementation in GNU Classpath 0.97.2 is buggy.
  JavaMethod* getAnnotation =
    UPCALL_METHOD(loader, "java/lang/reflect/AccessibleObject", "getAnnotation",
                  "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;",
                  ACC_VIRTUAL);
  getAnnotation->setCompiledPtr((void*)(intptr_t)nativeGetAnnotation,
                                "nativeGetAnnotation");
  
  JavaMethod* getAnnotations =
    UPCALL_METHOD(loader, "java/lang/reflect/AccessibleObject",
                  "getDeclaredAnnotations",
                  "()[Ljava/lang/annotation/Annotation;",
                  ACC_VIRTUAL);
  getAnnotations->setCompiledPtr((void*)(intptr_t)nativeGetDeclaredAnnotations,
                                 "nativeGetDeclaredAnnotations");
}

