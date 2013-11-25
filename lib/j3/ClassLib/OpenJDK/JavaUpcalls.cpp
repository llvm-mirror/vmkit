//===-------- JavaUpcalls.cpp - Upcalls to Java entities ------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JavaReferenceQueue.h"

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


using namespace j3;

Class*      Classpath::newThread;
JavaMethod* Classpath::initThread;
JavaMethod* Classpath::runThread;
JavaMethod* Classpath::groupAddThread;
JavaField*  Classpath::groupName;
JavaMethod* Classpath::initGroup;
JavaMethod* Classpath::initNamedGroup;
JavaField*  Classpath::priority;
JavaField*  Classpath::daemon;
JavaField*  Classpath::eetop;
JavaField*  Classpath::threadStatus;
JavaField*  Classpath::group;
Class*      Classpath::threadGroup;
JavaMethod* Classpath::getUncaughtExceptionHandler;
JavaMethod* Classpath::uncaughtException;
JavaMethod* Classpath::internString;

JavaMethod* Classpath::setContextClassLoader;
JavaMethod* Classpath::getSystemClassLoader;
Class*      Classpath::newString;
Class*      Classpath::newClass;
Class*      Classpath::newThrowable;
JavaField*  Classpath::backtrace;
JavaField*  Classpath::detailMessage;
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
/*
 * TODO Implement annotation support on openJDK.
 */
JavaField*  Classpath::vmThread;
JavaField*  Classpath::vmdataVMThread;
UserClass*	Classpath::newVMConstructor;
UserClass*	Classpath::newHashMap;
JavaMethod*	Classpath::initHashMap;
JavaMethod*	Classpath::putHashMap;
UserClass*	Classpath::newAnnotationHandler;
UserClass*	Classpath::newAnnotation;
JavaMethod*	Classpath::createAnnotation;
JavaMethod*	Classpath::getInField;
JavaMethod*	Classpath::getFieldInClass;
JavaField*	Classpath::threadName;
/***********************************************/
Class*      Classpath::newField;
Class*      Classpath::newMethod;
ClassArray* Classpath::methodArrayClass;
ClassArray* Classpath::fieldArrayClass;
JavaField*  Classpath::methodSlot;
JavaField*  Classpath::fieldSlot;
ClassArray* Classpath::classArrayClass;
JavaMethod* Classpath::loadInClassLoader;
JavaField*  Classpath::bufferAddress;
Class*      Classpath::newDirectByteBuffer;
JavaField*  Classpath::vmdataClassLoader;
JavaMethod* Classpath::InitDirectByteBuffer;
Class*      Classpath::newClassLoader;
Class*      Classpath::cloneableClass;
JavaMethod* Classpath::ReflectInvokeMethod;


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
Class* Classpath::InstantiationException;
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
Class* Classpath::ArithmeticException;
Class* Classpath::CloneNotSupportedException;

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
JavaMethod* Classpath::InitInstantiationException;
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
JavaMethod* Classpath::InitArithmeticException;
JavaMethod* Classpath::InitCloneNotSupportedException;
JavaMethod* Classpath::InitObject;
JavaMethod* Classpath::FinalizeObject;
JavaMethod* Classpath::IntToString;

JavaMethod* Classpath::SystemArraycopy;
Class*      Classpath::SystemClass;
JavaMethod* Classpath::SystemExit;
JavaMethod* Classpath::initSystem;
Class*      Classpath::EnumClass;
Class*      Classpath::assertionStatusDirectivesClass;

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

Class* Classpath::OfObject;

JavaField* Classpath::methodClass;
JavaField* Classpath::fieldClass;
JavaField* Classpath::constructorClass;
Class*     Classpath::constantPoolClass;
JavaField* Classpath::constantPoolOop;

JavaMethod* Classpath::EnqueueReference;
Class*      Classpath::newReference;
JavaField*  Classpath::NullRefQueue;
JavaField*  Classpath::RefLock;
Class*      Classpath::newRefLock;
JavaField*  Classpath::RefPending;
Class*      Classpath::RefHandlerClass;
JavaMethod* Classpath::initRefHandler;
JavaMethod* Classpath::threadStart;
JavaMethod* Classpath::threadExit;

const char * OpenJDKBootPath =
      OpenJDKJRE "/lib/rt.jar"
  ":" OpenJDKJRE "/lib/resources.jar"
  ":" OpenJDKJRE "/lib/jsse.jar"
  ":" OpenJDKJRE "/lib/jce.jar"
  ":" OpenJDKJRE "/lib/charsets.jar"
  ":" VMKitOpenJDKZip ;

void Classpath::CreateJavaThread(Jnjvm* vm, JavaThread* myth,
                                 const char* thName, JavaObject* Group) {
  JavaObject* th = NULL;
  JavaObject* name = NULL;
  JavaObject* sleep = NULL;
  llvm_gcroot(Group, 0);
  llvm_gcroot(th, 0);
  llvm_gcroot(name, 0);
  llvm_gcroot(sleep, 0);

  assert(thName && thName[0] && "Invalid thread name!");

  th = newThread->doNew(vm);
  sleep = OfObject->doNew(vm);
  myth->initialise(th, sleep);

  name = vm->asciizToStr(thName);

  // Initialize the values
  priority->setInstanceInt32Field(th, (uint32)5);
  daemon->setInstanceInt8Field(th, (uint32)false);

  // call Thread(ThreadGroup,String) constructor
  initThread->invokeIntSpecial(vm, newThread, th, &Group, &name);

  // Store reference to the JavaThread for this thread in the 'eetop' field
  // GC-safe since 'eetop' is of type 'long'
  eetop->setInstanceLongField(th, (long)myth);
}

void Classpath::InitializeThreading(Jnjvm* vm) {

  JavaObject* RG = 0;
  JavaObject* SystemGroup = 0;
  JavaObject* systemName = 0;
  JavaObject* MainGroup = 0;
  JavaObject* mainName = 0;
  JavaObject* RefHandler = 0;
  JavaObject* RefHandlerName = 0;
  llvm_gcroot(RG, 0);
  llvm_gcroot(SystemGroup, 0);
  llvm_gcroot(systemName, 0);
  llvm_gcroot(MainGroup, 0);
  llvm_gcroot(mainName, 0);
  llvm_gcroot(RefHandler, 0);
  llvm_gcroot(RefHandlerName, 0);

  newRefLock->resolveClass();
  newRefLock->initialiseClass(vm);
  RefHandlerClass->resolveClass();
  RefHandlerClass->initialiseClass(vm);

  // Resolve and initialize classes first.
  newThread->resolveClass();
  newThread->initialiseClass(vm);

  threadGroup->resolveClass();
  threadGroup->initialiseClass(vm);

  // Create the "system" group.
  SystemGroup = threadGroup->doNew(vm);
  initGroup->invokeIntSpecial(vm, threadGroup, SystemGroup);

  // Create the "main" group, child of the "system" group.
  MainGroup = threadGroup->doNew(vm);
  mainName = vm->asciizToStr("main");
  initNamedGroup->invokeIntSpecial(vm, threadGroup, MainGroup, &SystemGroup, &mainName);

  // Create the main thread
  assert(vm->getMainThread() && "VM did not set its main thread");
  CreateJavaThread(vm, (JavaThread*)vm->getMainThread(), "main", MainGroup);

  // Create the finalizer thread.
  assert(vm->getFinalizerThread() && "VM did not set its finalizer thread");
  CreateJavaThread(vm, vm->getFinalizerThread(), "Finalizer", SystemGroup);

  // Create the enqueue thread.
  assert(vm->getReferenceThread() && "VM did not set its enqueue thread");
  CreateJavaThread(vm, vm->getReferenceThread(), "Reference", SystemGroup);

  // Create the ReferenceHandler thread.
  RefHandler = RefHandlerClass->doNew(vm);
  RefHandlerName = vm->asciizToStr("Reference Handler");
  initRefHandler->invokeIntSpecial(vm, RefHandlerClass, RefHandler,
      &SystemGroup, &RefHandlerName);
  priority->setInstanceInt32Field(RefHandler, 10); // MAX_PRIORITY
  daemon->setInstanceInt8Field(RefHandler, (uint32)true);
  threadStart->invokeIntVirtual(vm, RefHandlerClass, RefHandler);
}

extern "C" void Java_java_lang_ref_WeakReference__0003Cinit_0003E__Ljava_lang_Object_2(
    JavaObjectReference* reference, JavaObject* referent) {
  llvm_gcroot(reference, 0);
  llvm_gcroot(referent, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObjectReference::init(reference, referent, 0);
  JavaThread::get()->getJVM()->getReferenceThread()->addWeakReference(reference);

  END_NATIVE_EXCEPTION

}

extern "C" void Java_java_lang_ref_WeakReference__0003Cinit_0003E__Ljava_lang_Object_2Ljava_lang_ref_ReferenceQueue_2(
    JavaObjectReference* reference,
    JavaObject* referent,
    JavaObject* queue) {
  llvm_gcroot(reference, 0);
  llvm_gcroot(referent, 0);
  llvm_gcroot(queue, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObjectReference::init(reference, referent, queue);
  JavaThread::get()->getJVM()->getReferenceThread()->addWeakReference(reference);

  END_NATIVE_EXCEPTION

}

extern "C" void Java_java_lang_ref_SoftReference__0003Cinit_0003E__Ljava_lang_Object_2(
    JavaObjectReference* reference, JavaObject* referent) {
  llvm_gcroot(reference, 0);
  llvm_gcroot(referent, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObjectReference::init(reference, referent, 0);
  JavaThread::get()->getJVM()->getReferenceThread()->addSoftReference(reference);

  END_NATIVE_EXCEPTION

}

extern "C" void Java_java_lang_ref_SoftReference__0003Cinit_0003E__Ljava_lang_Object_2Ljava_lang_ref_ReferenceQueue_2(
    JavaObjectReference* reference,
    JavaObject* referent,
    JavaObject* queue) {
  llvm_gcroot(reference, 0);
  llvm_gcroot(referent, 0);
  llvm_gcroot(queue, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObjectReference::init(reference, referent, queue);
  JavaThread::get()->getJVM()->getReferenceThread()->addSoftReference(reference);

  END_NATIVE_EXCEPTION

}

extern "C" void Java_java_lang_ref_PhantomReference__0003Cinit_0003E__Ljava_lang_Object_2Ljava_lang_ref_ReferenceQueue_2(
    JavaObjectReference* reference,
    JavaObject* referent,
    JavaObject* queue) {
  llvm_gcroot(reference, 0);
  llvm_gcroot(referent, 0);
  llvm_gcroot(queue, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObjectReference::init(reference, referent, queue);
  JavaThread::get()->getJVM()->getReferenceThread()->addPhantomReference(reference);

  END_NATIVE_EXCEPTION
}

extern "C" JavaObject* Java_sun_reflect_Reflection_getCallerClass__I(uint32 index) {

  JavaObject* res = 0;
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* cl = th->getCallingClassLevel(index);
  if (cl) res = cl->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* Java_java_lang_reflect_AccessibleObject_getAnnotation__Ljava_lang_Class_2(
    JavaObject* obj) {
  llvm_gcroot(obj, 0);
  return 0;
}

extern "C" JavaObject* Java_java_lang_reflect_AccessibleObject_getDeclaredAnnotations__() {
  JavaObject* res = 0;
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassArray* array = vm->upcalls->constructorArrayAnnotation;
  res = array->doNew(0, vm);

  END_NATIVE_EXCEPTION

  return res;
}

// The OpenJDK Reference clinit spawns a Thread!
// We split that functionality out until we have threading working.
extern "C" void Java_java_lang_ref_Reference__0003Cclinit_0003E() {
  JavaObject* lock = 0;
  llvm_gcroot(lock, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  Jnjvm* vm = JavaThread::get()->getJVM();

  // Initialize the static fields of this class, skip
  // the spawning of the reference queue thread.
  lock = vm->upcalls->newRefLock->doNew(vm);
  vm->upcalls->RefLock->setStaticObjectField(lock);
  vm->upcalls->RefPending->setStaticObjectField(NULL);

  END_NATIVE_EXCEPTION
}

extern "C" void nativeJavaObjectClassTracer(
    JavaObjectClass* obj, word_t closure) {
  llvm_gcroot(obj, 0);
  JavaObjectClass::staticTracer(obj, closure);
}

extern "C" void nativeJavaObjectFieldTracer(
    JavaObjectField* obj, word_t closure) {
  llvm_gcroot(obj, 0);
  JavaObjectField::staticTracer(obj, closure);
}

extern "C" void nativeJavaObjectMethodTracer(
    JavaObjectMethod* obj, word_t closure) {
  llvm_gcroot(obj, 0);
  JavaObjectMethod::staticTracer(obj, closure);
}

extern "C" void nativeJavaObjectConstructorTracer(
    JavaObjectConstructor* obj, word_t closure) {
  llvm_gcroot(obj, 0);
  JavaObjectConstructor::staticTracer(obj, closure);
}


void Classpath::initialiseClasspath(JnjvmClassLoader* loader) {

  // Load OpenJDK's libjava.so
  void * handle = loader->loadLib(OpenJDKLibJava);
  if (handle == NULL) {
      fprintf(stderr, "Failed to load %s, cannot proceed:\n%s\n", OpenJDKLibJava, loader->getErrorMessage());
    abort();
  }

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
  // Add space for extra fields, see ClasspathReflect.h
  newClass->virtualSize += 2*sizeof(void*);

  newThrowable =
    UPCALL_CLASS(loader, "java/lang/Throwable");

  backtrace =
    UPCALL_FIELD(loader, "java/lang/Throwable",
        "backtrace", "Ljava/lang/Object;", ACC_VIRTUAL);

  detailMessage =
    UPCALL_FIELD(loader, "java/lang/Throwable",
        "detailMessage", "Ljava/lang/String;", ACC_VIRTUAL);

  newException =
    UPCALL_CLASS(loader, "java/lang/Exception");

  newDirectByteBuffer =
    UPCALL_CLASS(loader, "java/nio/DirectByteBuffer");

  InitDirectByteBuffer =
    UPCALL_METHOD(loader, "java/nio/DirectByteBuffer", "<init>", "(JI)V",
                  ACC_VIRTUAL);

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
    "(Ljava/lang/Class;[Ljava/lang/Class;[Ljava/lang/Class;"
    "IILjava/lang/String;[B[B)V",
      ACC_VIRTUAL);

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
                  "(Ljava/lang/Class;Ljava/lang/String;[Ljava/lang/Class;"
                  "Ljava/lang/Class;[Ljava/lang/Class;IILjava/lang/String;"
                  "[B[B[B)V", ACC_VIRTUAL);

  newMethod =
    UPCALL_CLASS(loader, "java/lang/reflect/Method");

  methodArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/reflect/Method", 1);

  methodSlot =
    UPCALL_FIELD(loader, "java/lang/reflect/Method", "slot", "I", ACC_VIRTUAL);

  initField =
    UPCALL_METHOD(loader, "java/lang/reflect/Field", "<init>",
                  "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Class;IILjava/lang/String;[B)V",
                  ACC_VIRTUAL);

  newField =
    UPCALL_CLASS(loader, "java/lang/reflect/Field");

  fieldArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/reflect/Field", 1);

  fieldSlot =
    UPCALL_FIELD(loader, "java/lang/reflect/Field", "slot", "I", ACC_VIRTUAL);


  classArrayClass =
    UPCALL_ARRAY_CLASS(loader, "java/lang/Class", 1);

  bufferAddress =
    UPCALL_FIELD(loader, "java/nio/Buffer", "address", "J",
                 ACC_VIRTUAL);

  vmdataClassLoader =
    UPCALL_FIELD(loader, "java/lang/ClassLoader", "classes", "Ljava/util/Vector;",
                 ACC_VIRTUAL);

  newStackTraceElement =
    UPCALL_CLASS(loader, "java/lang/StackTraceElement");

  stackTraceArray =
    UPCALL_ARRAY_CLASS(loader, "java/lang/StackTraceElement", 1);

  initStackTraceElement =
    UPCALL_METHOD(loader,  "java/lang/StackTraceElement", "<init>",
                  "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V",
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

  Classpath::OfObject =
    UPCALL_CLASS(loader, "java/lang/Object");

  loadInClassLoader =
    UPCALL_METHOD(loader, "java/lang/ClassLoader", "loadClass",
                  "(Ljava/lang/String;)Ljava/lang/Class;", ACC_VIRTUAL);

  // Make sure classes the JIT optimizes on are loaded.
  // TODO: What to do with these? I think we can just remove them...
  //UPCALL_CLASS(loader, "java/lang/VMFloat");
  //UPCALL_CLASS(loader, "java/lang/VMDouble");

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
  UPCALL_CLASS_EXCEPTION(loader, InstantiationException);
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
  UPCALL_CLASS_EXCEPTION(loader, ArithmeticException);
  UPCALL_CLASS_EXCEPTION(loader, CloneNotSupportedException);

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
  UPCALL_METHOD_EXCEPTION(loader, InstantiationException);
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
  UPCALL_METHOD_EXCEPTION(loader, ArithmeticException);
  UPCALL_METHOD_EXCEPTION(loader, CloneNotSupportedException);

  UPCALL_METHOD_WITH_EXCEPTION(loader, NoClassDefFoundError);
  UPCALL_METHOD_WITH_EXCEPTION(loader, ExceptionInInitializerError);
  UPCALL_METHOD_WITH_EXCEPTION(loader, InvocationTargetException);

  InitObject = UPCALL_METHOD(loader, "java/lang/Object", "<init>", "()V",
                             ACC_VIRTUAL);

  FinalizeObject = UPCALL_METHOD(loader, "java/lang/Object", "finalize", "()V",
                                 ACC_VIRTUAL);

  IntToString = UPCALL_METHOD(loader, "java/lang/Integer", "toString",
                              "(II)Ljava/lang/String;", ACC_STATIC);

  SystemArraycopy = UPCALL_METHOD(loader, "java/lang/System", "arraycopy",
                                  "(Ljava/lang/Object;ILjava/lang/Object;II)V",
                                  ACC_STATIC);

  SystemClass = UPCALL_CLASS(loader, "java/lang/System");

  SystemExit = UPCALL_METHOD(loader, "java/lang/System", "exit",
            "(I)V", ACC_STATIC);

  initSystem =
    UPCALL_METHOD(loader, "java/lang/System", "initializeSystemClass", "()V",
        ACC_STATIC);

  SystemClass = UPCALL_CLASS(loader, "java/lang/System");
  EnumClass = UPCALL_CLASS(loader, "java/lang/Enum");

  assertionStatusDirectivesClass =
    UPCALL_CLASS(loader, "java/lang/AssertionStatusDirectives");

  cloneableClass = UPCALL_CLASS(loader, "java/lang/Cloneable");

  ReflectInvokeMethod =
    UPCALL_METHOD(loader, "java/lang/reflect/Method", "invoke",
      "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;", ACC_VIRTUAL);

  newThread =
    UPCALL_CLASS(loader, "java/lang/Thread");

  initThread =
    UPCALL_METHOD(loader, "java/lang/Thread", "<init>",
        "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V", ACC_VIRTUAL);

  runThread =
    UPCALL_METHOD(loader, "java/lang/Thread", "run", "()V", ACC_VIRTUAL);

  groupAddThread =
    UPCALL_METHOD(loader, "java/lang/ThreadGroup", "addThread",
                  "(Ljava/lang/Thread;)V", ACC_VIRTUAL);

  initGroup =
    UPCALL_METHOD(loader, "java/lang/ThreadGroup", "<init>",
                  "()V", ACC_VIRTUAL);

  initNamedGroup =
    UPCALL_METHOD(loader, "java/lang/ThreadGroup", "<init>",
                  "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V", ACC_VIRTUAL);

  groupName =
    UPCALL_FIELD(loader, "java/lang/ThreadGroup", "name", "Ljava/lang/String;",
                 ACC_VIRTUAL);
  priority =
    UPCALL_FIELD(loader,  "java/lang/Thread", "priority", "I", ACC_VIRTUAL);

  daemon =
    UPCALL_FIELD(loader, "java/lang/Thread", "daemon", "Z", ACC_VIRTUAL);

  eetop =
    UPCALL_FIELD(loader, "java/lang/Thread", "eetop", "J", ACC_VIRTUAL);

  threadStatus =
    UPCALL_FIELD(loader, "java/lang/Thread", "threadStatus", "I", ACC_VIRTUAL);
  group =
    UPCALL_FIELD(loader, "java/lang/Thread", "group",
                 "Ljava/lang/ThreadGroup;", ACC_VIRTUAL);

  threadGroup =
    UPCALL_CLASS(loader, "java/lang/ThreadGroup");

  // TODO: Verify this works in OpenJDK, merged from upstream
  getUncaughtExceptionHandler =
    UPCALL_METHOD(loader, "java/lang/Thread", "getUncaughtExceptionHandler",
                  "()Ljava/lang/Thread$UncaughtExceptionHandler;", ACC_VIRTUAL);

  uncaughtException =
    UPCALL_METHOD(loader, "java/lang/Thread$UncaughtExceptionHandler",
                  "uncaughtException",
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

  constantPoolClass =
    UPCALL_CLASS(loader, "sun/reflect/ConstantPool");
  constantPoolOop =
    UPCALL_FIELD(loader, "sun/reflect/ConstantPool", "constantPoolOop",
                 "Ljava/lang/Object;", ACC_VIRTUAL);

  loader->loadName(loader->asciizConstructUTF8("java/lang/String"),
                                       true, false, NULL);

  loader->loadName(loader->asciizConstructUTF8("java/lang/Object"),
                                       true, false, NULL);

  // Don't compile methods here, we still don't know where to allocate Java
  // strings.

  JavaMethod* getCallerClass =
    UPCALL_METHOD(loader, "sun/reflect/Reflection", "getCallerClass",
                  "(I)Ljava/lang/Class;", ACC_STATIC);
  getCallerClass->setNative();

  //===----------------------------------------------------------------------===//
  //
  // To make classes non GC-allocated, we have to bypass the tracer functions of
  // java.lang.Class, java.lang.reflect.Field, java.lang.reflect.Method and
  // java.lang.reflect.constructor. The new tracer functions trace the classloader
  // instead of the class/field/method.
  //
  //===----------------------------------------------------------------------===//

  newClass->getVirtualVT()->setNativeTracer(
      (word_t)nativeJavaObjectClassTracer,
       "nativeJavaObjectClassTracer");

  newConstructor->getVirtualVT()->setNativeTracer(
      (word_t)nativeJavaObjectConstructorTracer,
      "nativeJavaObjectConstructorTracer");

   newMethod->getVirtualVT()->setNativeTracer(
      (word_t)nativeJavaObjectMethodTracer,
      "nativeJavaObjectMethodTracer");

   newField->getVirtualVT()->setNativeTracer(
      (word_t)nativeJavaObjectFieldTracer,
      "nativeJavaObjectFieldTracer");

//   newVMConstructor->getVirtualVT()->setNativeTracer(
//         (word_t)nativeJavaObjectVMConstructorTracer,
//         "nativeJavaObjectVMConstructorTracer");
//
//      newVMMethod->getVirtualVT()->setNativeTracer(
//         (word_t)nativeJavaObjectVMMethodTracer,
//         "nativeJavaObjectVMMethodTracer");
//
//      newVMField->getVirtualVT()->setNativeTracer(
//         (word_t)nativeJavaObjectVMFieldTracer,
//         "nativeJavaObjectVMFieldTracer");

   //TODO: Fix native tracer for java.lang.Thread to not trace through
   // the eetop field to our internal JavaThread.
   //newVMThread->getVirtualVT()->setNativeTracer(
   //   (word_t)nativeJavaObjectVMThreadTracer,
   //   "nativeJavaObjectVMThreadTracer");

  newReference = UPCALL_CLASS(loader, "java/lang/ref/Reference");

  RefLock = UPCALL_FIELD(loader, "java/lang/ref/Reference",
      "lock", "Ljava/lang/ref/Reference$Lock;", ACC_STATIC);
  RefPending = UPCALL_FIELD(loader, "java/lang/ref/Reference",
      "pending", "Ljava/lang/ref/Reference;", ACC_STATIC);

  newRefLock = UPCALL_CLASS(loader, "java/lang/ref/Reference$Lock");

  RefHandlerClass =
    UPCALL_CLASS(loader, "java/lang/ref/Reference$ReferenceHandler");
  initRefHandler =
    UPCALL_METHOD(loader, "java/lang/ref/Reference$ReferenceHandler", "<init>",
        "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V", ACC_VIRTUAL);

  threadStart =
    UPCALL_METHOD(loader, "java/lang/Thread", "start",
        "()V", ACC_VIRTUAL);

  threadExit =
    UPCALL_METHOD(loader, "java/lang/Thread", "exit",
        "()V", ACC_VIRTUAL);

  EnqueueReference =
    UPCALL_METHOD(loader, "java/lang/ref/Reference",  "enqueue", "()Z",
                  ACC_VIRTUAL);

  NullRefQueue =
    UPCALL_FIELD(loader, "java/lang/ref/ReferenceQueue",
        "NULL", "Ljava/lang/ref/ReferenceQueue;", ACC_STATIC);

  JavaMethod* initWeakReference =
    UPCALL_METHOD(loader, "java/lang/ref/WeakReference", "<init>",
                  "(Ljava/lang/Object;)V",
                  ACC_VIRTUAL);
  initWeakReference->setNative();

  initWeakReference =
    UPCALL_METHOD(loader, "java/lang/ref/WeakReference", "<init>",
                  "(Ljava/lang/Object;Ljava/lang/ref/ReferenceQueue;)V",
                  ACC_VIRTUAL);
  initWeakReference->setNative();

  JavaMethod* initSoftReference =
    UPCALL_METHOD(loader, "java/lang/ref/SoftReference", "<init>",
                  "(Ljava/lang/Object;)V",
                  ACC_VIRTUAL);
  initSoftReference->setNative();

  initSoftReference =
    UPCALL_METHOD(loader, "java/lang/ref/SoftReference", "<init>",
                  "(Ljava/lang/Object;Ljava/lang/ref/ReferenceQueue;)V",
                  ACC_VIRTUAL);
  initSoftReference->setNative();

  JavaMethod* initPhantomReference =
    UPCALL_METHOD(loader, "java/lang/ref/PhantomReference", "<init>",
                  "(Ljava/lang/Object;Ljava/lang/ref/ReferenceQueue;)V",
                  ACC_VIRTUAL);
  initPhantomReference->setNative();

  JavaMethod * ReferenceClassInit =
    UPCALL_METHOD(loader, "java/lang/ref/Reference", "<clinit>",
                  "()V", ACC_STATIC);
  ReferenceClassInit->setNative();

  // String support

  internString =
    UPCALL_METHOD(loader, "java/lang/VMString", "intern",
        "(Ljava/lang/String;)Ljava/lang/String;", ACC_STATIC);
}

void Classpath::InitializeSystem(Jnjvm * jvm) {
  JavaObject * exc = NULL;
  llvm_gcroot(exc, 0);
  TRY {
    initSystem->invokeIntStatic(jvm, SystemClass);
  } CATCH {
    exc = JavaThread::get()->pendingException;
  } END_CATCH;

  if (exc) {
    fprintf(stderr, "Exception %s while initializing system.\n",
        UTF8Buffer(JavaObject::getClass(exc)->name).cString());
    JavaString * s = (JavaString*)detailMessage->getInstanceObjectField(exc);
    if (s) {
      fprintf(stderr, "Exception Message: \"%s\"\n",
          JavaString::strToAsciiz(s));
    }
    abort();
  }

  // resolve and initialize misc classes
  assertionStatusDirectivesClass->resolveClass();
  assertionStatusDirectivesClass->initialiseClass(jvm);

  constantPoolClass->resolveClass();
  constantPoolClass->initialiseClass(jvm);
}



#include "ClasspathConstructor.inc"
#include "ClasspathField.inc"
#include "ClasspathMethod.inc"
#include "OpenJDK.inc"
#include "Unsafe.inc"
#include "UnsafeForOpenJDK.inc"
