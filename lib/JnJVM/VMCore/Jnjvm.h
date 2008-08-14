//===---------- Jnjvm.h - Java virtual machine description ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_VM_H
#define JNJVM_JAVA_VM_H

#include <vector>

#include "types.h"

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "JavaAllocator.h"

namespace jnjvm {

class ArrayUInt8;
class Attribut;
class Class;
class ClassArray;
class CommonClass;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaString;
class JnjvmClassLoader;
class JnjvmModule;
class JnjvmModuleProvider;
class Reader;
class Typedef;
class UTF8;
class UTF8Map;
class ClassMap;
class DelegateeMap;
class FieldMap;
class MethodMap;
class Signdef;
class SignMap;
class StaticInstanceMap;
class StringMap;
class TypeMap;
class FunctionMap;
class FunctionDefMap;
class FunctionDefMap;
class AllocationMap;
class ZipArchive;

class Jnjvm : public mvm::VirtualMachine {
public:
#ifdef MULTIPLE_GC
  Collector* GC;
#endif
  JavaAllocator allocator;
  static VirtualTable* VT;

  static const char* dirSeparator;
  static const char* envSeparator;
  static const unsigned int Magic;
  
  // Misc constants
  static const double MaxDouble;
  static const double MinDouble;
  static const double MaxLongDouble;
  static const double MinLongDouble;
  static const double MaxIntDouble;
  static const double MinIntDouble;
  static const uint64 MaxLong;
  static const uint64 MinLong;
  static const uint32 MaxInt;
  static const uint32 MinInt;
  static const float MaxFloat;
  static const float MinFloat;
  static const float MaxIntFloat;
  static const float MinIntFloat;
  static const float MaxLongFloat;
  static const float MinLongFloat;
  static const float NaNFloat;
  static const double NaNDouble;

  // Exceptions name
  static const char* ArithmeticException;
  static const char* ClassNotFoundException;
  static const char* InvocationTargetException;
  static const char* ArrayStoreException;
  static const char* ClassCastException;
  static const char* IllegalMonitorStateException;
  static const char* IllegalArgumentException;
  static const char* InterruptedException;
  static const char* IndexOutOfBoundsException;
  static const char* ArrayIndexOutOfBoundsException;
  static const char* NegativeArraySizeException;
  static const char* NullPointerException;
  static const char* SecurityException;
  static const char* ClassFormatError;
  static const char* ClassCircularityError;
  static const char* NoClassDefFoundError;
  static const char* UnsupportedClassVersionError;
  static const char* NoSuchFieldError;
  static const char* NoSuchMethodError;
  static const char* InstantiationError;
  static const char* IllegalAccessError;
  static const char* IllegalAccessException;
  static const char* VerifyError;
  static const char* ExceptionInInitializerError;
  static const char* LinkageError;
  static const char* AbstractMethodError;
  static const char* UnsatisfiedLinkError;
  static const char* InternalError;
  static const char* OutOfMemoryError;
  static const char* StackOverflowError;
  static const char* UnknownError;

  
  // Exceptions
  void arrayStoreException();
  void indexOutOfBounds(const JavaObject* obj, sint32 entry);
  void negativeArraySizeException(int size);
  void nullPointerException(const char* fmt, ...);
  void illegalAccessException(const char* msg);
  void illegalMonitorStateException(const JavaObject* obj);
  void interruptedException(const JavaObject* obj);
  void initializerError(const JavaObject* excp);
  void invocationTargetException(const JavaObject* obj);
  void outOfMemoryError(sint32 n);
  void illegalArgumentExceptionForMethod(JavaMethod* meth, CommonClass* required,
                                         CommonClass* given);
  void illegalArgumentExceptionForField(JavaField* field, CommonClass* required,
                                        CommonClass* given);
  void illegalArgumentException(const char* msg);
  void classCastException(const char* msg);
  void unknownError(const char* fmt, ...); 
  void error(const char* className, const char* fmt, ...);
  void verror(const char* className, const char* fmt, va_list ap);
  void errorWithExcp(const char* className, const JavaObject* excp);

  
  static const UTF8* initName;
  static const UTF8* clinitName;
  static const UTF8* clinitType; 
  static const UTF8* runName; 
  static const UTF8* prelib; 
  static const UTF8* postlib; 
  static const UTF8* mathName;
  static const UTF8* abs;
  static const UTF8* sqrt;
  static const UTF8* sin;
  static const UTF8* cos;
  static const UTF8* tan;
  static const UTF8* asin;
  static const UTF8* acos;
  static const UTF8* atan;
  static const UTF8* atan2;
  static const UTF8* exp;
  static const UTF8* log;
  static const UTF8* pow;
  static const UTF8* ceil;
  static const UTF8* floor;
  static const UTF8* rint;
  static const UTF8* cbrt;
  static const UTF8* cosh;
  static const UTF8* expm1;
  static const UTF8* hypot;
  static const UTF8* log10;
  static const UTF8* log1p;
  static const UTF8* sinh;
  static const UTF8* tanh;
  static const UTF8* finalize;

 
  void initialiseClass(CommonClass* cl);
  
  JavaString* asciizToStr(const char* asciiz);
  JavaString* UTF8ToStr(const UTF8* utf8);
  
  
  JavaObject* getClassDelegatee(CommonClass*);

  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Jnjvm<>");
  }
  
  ~Jnjvm();
  Jnjvm();

  void addProperty(char* key, char* value);
 
  void* jniEnv;
  const void* javavmEnv;
  std::vector< std::pair<char*, char*> > postProperties;
  std::vector<void*> nativeLibs;
  const char* classpath;


  std::vector<JavaObject*, gc_allocator<JavaObject*> > globalRefs;
  mvm::Lock* globalRefsLock;

  void setClasspath(char* cp) {
    classpath = cp;
  }

  const char* name;
  JnjvmClassLoader* appClassLoader;
  StringMap * hashStr;
#ifdef MULTIPLE_VM
  StaticInstanceMap* statics;
  DelegateeMap* delegatees;
#endif

  
  virtual void runApplication(int argc, char** argv);
};

} // end namespace jnjvm

#endif
