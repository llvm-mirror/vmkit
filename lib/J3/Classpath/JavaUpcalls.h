//===---------- JavaUpcalls.h - Upcalls to Java entities ------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_UPCALLS_H
#define JNJVM_JAVA_UPCALLS_H

#include "mvm/Allocator.h"

#include "JnjvmConfig.h"

#define UPCALL_CLASS(vm, name)                                                 \
  vm->loadName(vm->asciizConstructUTF8(name), true, false, NULL)

#define UPCALL_PRIMITIVE_CLASS(loader, name, nb)                               \
  new(loader->allocator, "Primitive class")                                    \
          UserClassPrimitive(loader, loader->asciizConstructUTF8(name), nb)    \

#define UPCALL_FIELD(vm, cl, name, type, acc)                                  \
  UPCALL_CLASS(vm, cl)->lookupFieldDontThrow(vm->asciizConstructUTF8(name),    \
                                             vm->asciizConstructUTF8(type),    \
                                             isStatic(acc), false, 0)

#define UPCALL_METHOD(vm, cl, name, type, acc)                                 \
  UPCALL_CLASS(vm, cl)->lookupMethodDontThrow(vm->asciizConstructUTF8(name),   \
                                              vm->asciizConstructUTF8(type),   \
                                              isStatic(acc), false, 0)

#define UPCALL_ARRAY_CLASS(loader, name, depth)                                \
  loader->constructArray(                                                      \
    loader->constructArrayName(depth, loader->asciizConstructUTF8(name)))       

#define UPCALL_CLASS_EXCEPTION(loader, name)                                   \
  name = UPCALL_CLASS(loader, "java/lang/"#name)                           

#define UPCALL_REFLECT_CLASS_EXCEPTION(loader, name)                           \
  name = UPCALL_CLASS(loader, "java/lang/reflect/"#name)                   

#define UPCALL_METHOD_EXCEPTION(loader, name) \
  Init##name = name->lookupMethodDontThrow(loader->asciizConstructUTF8("<init>"), \
                                           loader->asciizConstructUTF8("(Ljava/lang/String;)V"), \
                                           false, false, 0);

#define UPCALL_METHOD_WITH_EXCEPTION(loader, name) \
  ErrorWithExcp##name = name->lookupMethodDontThrow(loader->asciizConstructUTF8("<init>"), \
                                     loader->asciizConstructUTF8("(Ljava/lang/Throwable;)V"), \
                                     false, false, 0);

namespace j3 {

class Jnjvm;
class JavaField;
class JavaMethod;
class Class;
class ClassArray;

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#define DYLD_EXTENSION ".dylib"
#else
#define SELF_HANDLE 0
#define DYLD_EXTENSION ".so"
#endif

class Classpath : public mvm::PermanentObject {
public: 
	Classpath(JnjvmBootstrapLoader* loader, bool dlLoad);

  void postInitialiseClasspath(JnjvmClassLoader* loader);

  UserClass*  newClassLoader;
  JavaMethod* getSystemClassLoader;
  JavaMethod* setContextClassLoader;
  UserClass* newString;
  UserClass* newClass;
  UserClass* newThrowable;
  UserClass* newException;
  JavaMethod* initClass;
  JavaMethod* initClassWithProtectionDomain;
  JavaField* vmdataClass;
  JavaMethod* setProperty;
  JavaMethod* initString;
  JavaMethod* getCallingClassLoader;
  JavaMethod* initConstructor;
  UserClassArray* constructorArrayClass;
  UserClassArray* constructorArrayAnnotation;
  UserClass*      newConstructor;
  JavaField*  constructorSlot;
  JavaMethod* initMethod;
  JavaMethod* initField;
  UserClassArray* methodArrayClass;
  UserClassArray* fieldArrayClass;
  UserClass*      newMethod;
  UserClass*      newField;
  JavaField*  methodSlot;
  JavaField*  fieldSlot;
  UserClassArray* classArrayClass;
  JavaMethod* loadInClassLoader;
  JavaMethod* initVMThrowable;
  JavaField*  vmDataVMThrowable;
  UserClass*  newVMThrowable;
  JavaField*  bufferAddress;
  JavaField*  dataPointer32;
  JavaField*  dataPointer64;
  UserClass*  newPointer32;
  UserClass*  newPointer64;
  UserClass*  newDirectByteBuffer;
  JavaMethod* InitDirectByteBuffer;
  JavaField*  vmdataClassLoader;

  JavaField* boolValue;
  JavaField* byteValue;
  JavaField* shortValue;
  JavaField* charValue;
  JavaField* intValue;
  JavaField* longValue;
  JavaField* floatValue;
  JavaField* doubleValue;

  UserClass* newStackTraceElement;
  UserClassArray* stackTraceArray;
  JavaMethod* initStackTraceElement;
  
  UserClass* voidClass;
  UserClass* boolClass;
  UserClass* byteClass;
  UserClass* shortClass;
  UserClass* charClass;
  UserClass* intClass;
  UserClass* floatClass;
  UserClass* doubleClass;
  UserClass* longClass;
  
  UserClass* vmStackWalker;
  
  UserClass* newThread;
  UserClass* newVMThread;
  JavaField* assocThread;
  JavaField* vmdataVMThread;
  JavaMethod* finaliseCreateInitialThread;
  JavaMethod* initVMThread;
  JavaMethod* runVMThread;
  JavaMethod* groupAddThread;
  JavaField* threadName;
  JavaField* groupName;
  JavaMethod* initGroup;
  JavaField* priority;
  JavaField* daemon;
  JavaField* group;
  JavaField* running;
  UserClass* threadGroup;
  JavaField* rootGroup;
  JavaField* vmThread;
  JavaMethod* uncaughtException;
  UserClass*  inheritableThreadLocal;
  

  UserClass* InvocationTargetException;
  UserClass* ArrayStoreException;
  UserClass* ClassCastException;
  UserClass* IllegalMonitorStateException;
  UserClass* IllegalArgumentException;
  UserClass* InterruptedException;
  UserClass* IndexOutOfBoundsException;
  UserClass* ArrayIndexOutOfBoundsException;
  UserClass* NegativeArraySizeException;
  UserClass* NullPointerException;
  UserClass* SecurityException;
  UserClass* ClassFormatError;
  UserClass* ClassCircularityError;
  UserClass* NoClassDefFoundError;
  UserClass* UnsupportedClassVersionError;
  UserClass* NoSuchFieldError;
  UserClass* NoSuchMethodError;
  UserClass* InstantiationError;
  UserClass* InstantiationException;
  UserClass* IllegalAccessError;
  UserClass* IllegalAccessException;
  UserClass* VerifyError;
  UserClass* ExceptionInInitializerError;
  UserClass* LinkageError;
  UserClass* AbstractMethodError;
  UserClass* UnsatisfiedLinkError;
  UserClass* InternalError;
  UserClass* OutOfMemoryError;
  UserClass* StackOverflowError;
  UserClass* UnknownError;
  UserClass* ClassNotFoundException;
  UserClass* ArithmeticException;

  JavaMethod* InitInvocationTargetException;
  JavaMethod* InitArrayStoreException;
  JavaMethod* InitClassCastException;
  JavaMethod* InitIllegalMonitorStateException;
  JavaMethod* InitIllegalArgumentException;
  JavaMethod* InitInterruptedException;
  JavaMethod* InitIndexOutOfBoundsException;
  JavaMethod* InitArrayIndexOutOfBoundsException;
  JavaMethod* InitNegativeArraySizeException;
  JavaMethod* InitNullPointerException;
  JavaMethod* InitSecurityException;
  JavaMethod* InitClassFormatError;
  JavaMethod* InitClassCircularityError;
  JavaMethod* InitNoClassDefFoundError;
  JavaMethod* InitUnsupportedClassVersionError;
  JavaMethod* InitNoSuchFieldError;
  JavaMethod* InitNoSuchMethodError;
  JavaMethod* InitInstantiationError;
  JavaMethod* InitInstantiationException;
  JavaMethod* InitIllegalAccessError;
  JavaMethod* InitIllegalAccessException;
  JavaMethod* InitVerifyError;
  JavaMethod* InitExceptionInInitializerError;
  JavaMethod* InitLinkageError;
  JavaMethod* InitAbstractMethodError;
  JavaMethod* InitUnsatisfiedLinkError;
  JavaMethod* InitInternalError;
  JavaMethod* InitOutOfMemoryError;
  JavaMethod* InitStackOverflowError;
  JavaMethod* InitUnknownError;
  JavaMethod* InitClassNotFoundException;
  JavaMethod* InitArithmeticException;
  
  JavaMethod* SystemArraycopy;
  JavaMethod* VMSystemArraycopy;
  Class*      SystemClass;
  
  JavaMethod* IntToString;

  JavaMethod* InitObject;
  JavaMethod* FinalizeObject;

  JavaMethod* ErrorWithExcpNoClassDefFoundError;
  JavaMethod* ErrorWithExcpExceptionInInitializerError;
  JavaMethod* ErrorWithExcpInvocationTargetException;
  
  

  UserClassArray* ArrayOfByte;
  UserClassArray* ArrayOfChar;
  UserClassArray* ArrayOfInt;
  UserClassArray* ArrayOfShort;
  UserClassArray* ArrayOfBool;
  UserClassArray* ArrayOfLong;
  UserClassArray* ArrayOfFloat;
  UserClassArray* ArrayOfDouble;
  UserClassArray* ArrayOfObject;
  UserClassArray* ArrayOfString;
  
  UserClassPrimitive* OfByte;
  UserClassPrimitive* OfChar;
  UserClassPrimitive* OfInt;
  UserClassPrimitive* OfShort;
  UserClassPrimitive* OfBool;
  UserClassPrimitive* OfLong;
  UserClassPrimitive* OfFloat;
  UserClassPrimitive* OfDouble;
  UserClassPrimitive* OfVoid;

  UserClass* OfObject;
  
  JavaField* methodClass;
  JavaField* fieldClass;
  JavaField* constructorClass;
  
  JavaMethod* EnqueueReference;
  Class*      newReference;

  /// upcalls - Upcall classes, fields and methods so that C++ code can call
  /// Java code.
  ///
  Classpath* upcalls;
  
  /// InterfacesArray - The interfaces that array classes implement.
  ///
  UserClass** InterfacesArray;

  /// SuperArray - The super of array classes.
  UserClass* SuperArray;

  /// Lists of UTF8s used internaly in VMKit.
  const UTF8* NoClassDefFoundErrorName;
  const UTF8* initName;
  const UTF8* clinitName;
  const UTF8* clinitType; 
  const UTF8* initExceptionSig;
  const UTF8* runName; 
  const UTF8* prelib; 
  const UTF8* postlib; 
  const UTF8* mathName;
  const UTF8* stackWalkerName;
  const UTF8* abs;
  const UTF8* sqrt;
  const UTF8* sin;
  const UTF8* cos;
  const UTF8* tan;
  const UTF8* asin;
  const UTF8* acos;
  const UTF8* atan;
  const UTF8* atan2;
  const UTF8* exp;
  const UTF8* log;
  const UTF8* pow;
  const UTF8* ceil;
  const UTF8* floor;
  const UTF8* rint;
  const UTF8* cbrt;
  const UTF8* cosh;
  const UTF8* expm1;
  const UTF8* hypot;
  const UTF8* log10;
  const UTF8* log1p;
  const UTF8* sinh;
  const UTF8* tanh;
  const UTF8* finalize;

  /// primitiveMap - Map of primitive classes, hashed by id.
  std::map<const char, UserClassPrimitive*> primitiveMap;

  UserClassPrimitive* getPrimitiveClass(char id) {
    return primitiveMap[id];
  }

  /// arrayTable - Table of array classes.
  UserClassArray* arrayTable[8];

  UserClassArray* getArrayClass(unsigned id) {
    return arrayTable[id - 4];
  }

private:
  void CreateJavaThread(Jnjvm* vm, JavaThread* myth,
                                       const char* name, JavaObject* Group);

public:
  void InitializeThreading(Jnjvm* vm);

  void CreateForeignJavaThread(Jnjvm* vm, JavaThread* myth);
};


} // end namespace j3

#endif
