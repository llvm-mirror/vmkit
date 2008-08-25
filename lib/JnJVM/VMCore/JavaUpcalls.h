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


#define UPCALL_CLASS(vm, name)                                             \
  vm->loadName(vm->asciizConstructUTF8(name), false, false)                        

#define UPCALL_FIELD(vm, cl, name, type, acc)                              \
  UPCALL_CLASS(vm, cl)->constructField(vm->asciizConstructUTF8(name),      \
                                       vm->asciizConstructUTF8(type), acc)

#define UPCALL_METHOD(vm, cl, name, type, acc)                             \
  UPCALL_CLASS(vm, cl)->constructMethod(vm->asciizConstructUTF8(name),     \
                                        vm->asciizConstructUTF8(type), acc)

#define UPCALL_ARRAY_CLASS(vm, name, depth)                                \
  vm->constructArray(                                                      \
    AssessorDesc::constructArrayName(vm, 0, depth,                         \
                                     vm->asciizConstructUTF8(name)))       

#define UPCALL_CLASS_EXCEPTION(loader, name)                               \
  name = UPCALL_CLASS(loader, "java/lang/"#name)                           

#define UPCALL_REFLECT_CLASS_EXCEPTION(loader, name)                       \
  name = UPCALL_CLASS(loader, "java/lang/reflect/"#name)                   

#define UPCALL_METHOD_EXCEPTION(loader, name) \
  Init##name = name->constructMethod(loader->asciizConstructUTF8("<init>"), \
                                     loader->asciizConstructUTF8("(Ljava/lang/String;)V"), \
                                     ACC_VIRTUAL);

#define UPCALL_METHOD_WITH_EXCEPTION(loader, name) \
  ErrorWithExcp##name = name->constructMethod(loader->asciizConstructUTF8("<init>"), \
                                     loader->asciizConstructUTF8("(Ljava/lang/Throwable;)V"), \
                                     ACC_VIRTUAL);

namespace jnjvm {

class Jnjvm;
class JavaField;
class JavaMethod;
class Class;
class ClassArray;

class ClasspathThread {
public:
  static void initialise(JnjvmClassLoader* vm);
  
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

class ClasspathException {
public:
  static void initialise(JnjvmClassLoader* vm);
  
  static Class* InvocationTargetException;
  static Class* ArrayStoreException;
  static Class* ClassCastException;
  static Class* IllegalMonitorStateException;
  static Class* IllegalArgumentException;
  static Class* InterruptedException;
  static Class* IndexOutOfBoundsException;
  static Class* ArrayIndexOutOfBoundsException;
  static Class* NegativeArraySizeException;
  static Class* NullPointerException;
  static Class* SecurityException;
  static Class* ClassFormatError;
  static Class* ClassCircularityError;
  static Class* NoClassDefFoundError;
  static Class* UnsupportedClassVersionError;
  static Class* NoSuchFieldError;
  static Class* NoSuchMethodError;
  static Class* InstantiationError;
  static Class* IllegalAccessError;
  static Class* IllegalAccessException;
  static Class* VerifyError;
  static Class* ExceptionInInitializerError;
  static Class* LinkageError;
  static Class* AbstractMethodError;
  static Class* UnsatisfiedLinkError;
  static Class* InternalError;
  static Class* OutOfMemoryError;
  static Class* StackOverflowError;
  static Class* UnknownError;
  static Class* ClassNotFoundException;

  static JavaMethod* InitInvocationTargetException;
  static JavaMethod* InitArrayStoreException;
  static JavaMethod* InitClassCastException;
  static JavaMethod* InitIllegalMonitorStateException;
  static JavaMethod* InitIllegalArgumentException;
  static JavaMethod* InitInterruptedException;
  static JavaMethod* InitIndexOutOfBoundsException;
  static JavaMethod* InitArrayIndexOutOfBoundsException;
  static JavaMethod* InitNegativeArraySizeException;
  static JavaMethod* InitNullPointerException;
  static JavaMethod* InitSecurityException;
  static JavaMethod* InitClassFormatError;
  static JavaMethod* InitClassCircularityError;
  static JavaMethod* InitNoClassDefFoundError;
  static JavaMethod* InitUnsupportedClassVersionError;
  static JavaMethod* InitNoSuchFieldError;
  static JavaMethod* InitNoSuchMethodError;
  static JavaMethod* InitInstantiationError;
  static JavaMethod* InitIllegalAccessError;
  static JavaMethod* InitIllegalAccessException;
  static JavaMethod* InitVerifyError;
  static JavaMethod* InitExceptionInInitializerError;
  static JavaMethod* InitLinkageError;
  static JavaMethod* InitAbstractMethodError;
  static JavaMethod* InitUnsatisfiedLinkError;
  static JavaMethod* InitInternalError;
  static JavaMethod* InitOutOfMemoryError;
  static JavaMethod* InitStackOverflowError;
  static JavaMethod* InitUnknownError;
  static JavaMethod* InitClassNotFoundException;

  static JavaMethod* ErrorWithExcpNoClassDefFoundError;
  static JavaMethod* ErrorWithExcpExceptionInInitializerError;
  static JavaMethod* ErrorWithExcpInvocationTargetException;
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
  static ClassArray* constructorArrayAnnotation;
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
  static ClassArray* stackTraceArray;
  static JavaMethod* initStackTraceElement;

  static void initialiseClasspath(JnjvmClassLoader* loader);
  
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
