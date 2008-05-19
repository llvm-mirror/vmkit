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

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#ifdef MULTIPLE_GC
#define vm_new(vm, cl) collector_new(cl, vm->GC)
#else
#define vm_new(vm, cl) gc_new(cl)
#endif

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
class StaticInstanceMap;
class StringMap;
class TypeMap;
class FunctionMap;
class FunctionDefMap;
class FunctionDefMap;
class AllocationMap;


class Jnjvm : public mvm::Object{
public:
#ifdef MULTIPLE_GC
  Collector* GC;
#endif
  static VirtualTable* VT;
  static Jnjvm* bootstrapVM;

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


  void analyseClasspathEnv(const char*);
  
  // Loads a Class
  CommonClass* loadName(const UTF8* name, JavaObject* loader, bool doResolve,
                        bool doClinit, bool doThrow);
  
  // Class lookup
  CommonClass* lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                   unsigned int len, JavaObject* loader,
                                   bool doResolve, bool doClinit, bool doThrow);
  CommonClass* lookupClassFromJavaString(JavaString* str, JavaObject* loader,
                                         bool doResolve, bool doClinit,
                                         bool doThrow);

  void readParents(Class* cl, Reader* reader);
  void loadParents(Class* cl);
  void readAttributs(Class* cl, Reader* reader,
                     std::vector<Attribut*, gc_allocator<Attribut*> > & attr);
  void readFields(Class* cl, Reader* reader);
  void readMethods(Class* cl, Reader* reader);
  void readClass(Class* cl);
  void initialiseClass(CommonClass* cl);
  void resolveClass(CommonClass* cl, bool doClinit);
  ArrayUInt8* openName(const UTF8* utf8);
  
  CommonClass* lookupClass(const UTF8* utf8, JavaObject* loader);
  JavaField* lookupField(CommonClass* cl, const UTF8* name, const UTF8* type);

  ClassArray* constructArray(const UTF8* name, JavaObject* loader);
  Class*      constructClass(const UTF8* name, JavaObject* loader);
  JavaField*  constructField(Class* cl, const UTF8* name, const UTF8* type,
                             uint32 access);
  JavaMethod* constructMethod(Class* cl, const UTF8* name, const UTF8* type,
                             uint32 access);
  const UTF8* asciizConstructUTF8(const char* asciiz);
  const UTF8* readerConstructUTF8(const uint16* buf, uint32 len);
  JavaString* asciizToStr(const char* asciiz);
  JavaString* UTF8ToStr(const UTF8* utf8);
  Typedef* constructType(const UTF8 * name);
  
  
  JavaObject* getClassDelegatee(CommonClass*);
  CommonClass* loadInClassLoader(const UTF8* utf8, JavaObject* loader);

  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Jnjvm<>");
  }
  
  virtual void destroyer(size_t sz);

  void addProperty(char* key, char* value);
 
  void* jniEnv;
  const void* javavmEnv;
  std::vector< std::pair<char*, char*> > postProperties;
  std::vector<char*> bootClasspath;
  std::vector<void*> nativeLibs;
  const char* classpath;
  const char* libClasspathEnv;
  const char* bootClasspathEnv;
  std::vector<JavaObject*, gc_allocator<JavaObject*> > globalRefs;
  mvm::Lock* globalRefsLock;

  void setClasspath(char* cp) {
    classpath = cp;
  }

  const char* name;
  JavaObject* appClassLoader;
  UTF8Map * hashUTF8;
  StringMap * hashStr;
  ClassMap* bootstrapClasses;
  MethodMap* loadedMethods;
  FieldMap* loadedFields;
  TypeMap* javaTypes;
#ifdef MULTIPLE_VM
  StaticInstanceMap* statics;
  DelegateeMap* delegatees;
#endif

  
  JnjvmModuleProvider* TheModuleProvider;
  JnjvmModule*         TheModule;


#ifndef MULTIPLE_GC
  void* allocateObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT);
  }
#else
  void* allocateObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT, GC);
  }
#endif
};

} // end namespace jnjvm

#endif
