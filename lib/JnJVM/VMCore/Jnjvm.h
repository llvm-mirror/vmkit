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

#include "mvm/Allocator.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "JavaTypes.h"
#include "JnjvmConfig.h"

namespace jnjvm {

class ArrayObject;
class Classpath;
class CommonClass;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaString;
class JavaThread;
class JnjvmBootstrapLoader;
class JnjvmClassLoader;
class StringMap;
class UserClass;
class UserClassArray;
class UserClassPrimitive;
class UserCommonClass;
class UTF8;

/// ThreadSystem - Thread management of a JVM. Each JVM has one thread
/// management system to count the number of non-daemon threads it owns.
/// The initial thread of the JVM is a non-daemon thread. When there are
/// no more non-daemon threads, the JVM stops executing.
///
class ThreadSystem {
public:
  /// nonDaemonThreads - Number of threads in the system that are not daemon
  /// threads.
  //
  uint16 nonDaemonThreads;

  /// nonDaemonLock - Protection lock for the nonDaemonThreads variable.
  ///
  mvm::LockNormal nonDaemonLock;

  /// nonDaemonVar - Condition variable to wake up the initial thread when it
  /// waits for other non-daemon threads to end. The non-daemon thread that
  /// decrements the nonDaemonThreads variable to zero wakes up the initial
  /// thread.
  ///
  mvm::Cond nonDaemonVar;
  
  /// ThreadSystem - Allocates a thread system management, initializing the
  /// lock, the condition variable and setting the initial number of non
  /// daemon threads to one, for the initial thread.
  ///
  ThreadSystem() {
    nonDaemonThreads = 1;
  }

  /// ~ThreadSystem - Destroys the thread system manager. Destroys the lock and
  /// the condition variable.
  ///
  ~ThreadSystem() {}

};

/// Jnjvm - A JVM. Each execution of a program allocates a Jnjvm.
///
class Jnjvm : public mvm::VirtualMachine {
public:
  /// allocator - Memory allocator of this JVM.
  ///
  mvm::Allocator* allocator;
#ifdef ISOLATE_SHARING
  UserClass* throwable;
#endif
  std::map<const char, UserClassArray*> arrayClasses;
private:
  
  ISOLATE_STATIC std::map<const char, UserClassPrimitive*> primitiveMap;

  /// bootstrapThread - The initial thread of this JVM.
  ///
  JavaThread* bootstrapThread;
  
  /// error - Throws an exception in the execution of a JVM for the thread
  /// that calls this functions. This is used internally by Jnjvm to control
  /// which pair class/method are used.
  ///
  void error(UserClass* cl, JavaMethod* meth, const char* fmt, ...);
  
  /// errorWithExcp - Throws an exception whose cause is the Java object excp.
  ///
  void errorWithExcp(UserClass* cl, JavaMethod* meth, const JavaObject* excp);
  
  /// loadAppClassLoader - Loads the application class loader, so that VMKit
  /// knowns which loader has to load the main class.
  ///
  JnjvmClassLoader* loadAppClassLoader();
  
  /// mapInitialThread - Maps the initial native thread to a java/lang/Thread
  /// object.
  ///
  void mapInitialThread();

  /// loadBootstrap - Bootstraps the JVM, getting the class loader, initializing
  /// bootstrap classes (e.g. java/lang/Class, java/lang/*Exception) and
  /// mapping the initial thread.
  ///
  void loadBootstrap();

  /// executeClass - Executes in the given JVM this main class with the given
  /// Java args.
  ///
  void executeClass(const char* className, ArrayObject* args);

  /// executePremain - Executes the premain class for the java/lang/instrument
  /// feature.
  ///
  void executePremain(const char* className, JavaString* args,
                      JavaObject* instrumenter);
  
  /// waitForExit - Waits that there are no more non-daemon threads in this JVM.
  ///
  void waitForExit();
  
  /// runMain - Runs the application with the given command line.
  ///
  void runMain(int argc, char** argv);


public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;
  
  /// print - Prints the JVM for debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer* buf) const;

  /// tracer - Traces instances of this class.
  ///
  virtual void TRACER;
  
  /// dirSeparator - Directory separator for file paths, e.g. '\' for windows,
  /// '/' for Unix.
  ///
  static const char* dirSeparator;

  /// envSeparator - Paths separator for environment variables, e.g. ':'.
  ///
  static const char* envSeparator;

  /// Magic - The magic number at the beginning of each .class file. 0xcafebabe.
  ///
  static const unsigned int Magic;
  
  /// Lists of UTF8s used internaly in VMKit.
  static const UTF8* NoClassDefFoundError;
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
 
  /// bootstraLoader - Bootstrap loader for base classes of this virtual
  /// machine.
  ///
  ISOLATE_STATIC JnjvmBootstrapLoader* bootstrapLoader;

  /// upcalls - Upcalls to call Java methods and access Java fields.
  ///
  Classpath* upcalls;

  /// threadSystem - The thread system to manage non-daemon threads and
  /// control the end of the JVM's execution.
  ///
  ThreadSystem threadSystem;
 
  /// jniEnv - The JNI environment of this JVM.
  ///
  void* jniEnv;

  /// javavmEnv - The Java VM environment of this JVM.
  ///
  const void* javavmEnv;

  /// postProperties - Properties set at runtime and in command line.
  ///
  std::vector< std::pair<char*, char*> > postProperties;

  /// nativeLibs - Native libraries (e.g. '.so') loaded by this JVM.
  ///
  ISOLATE_STATIC std::vector<void*> nativeLibs;

  /// classpath - The CLASSPATH value, or the paths given in command line.
  ///
  const char* classpath;

  /// globalRefs - Global references that JNI wants to protect.
  ///
  std::vector<JavaObject*, gc_allocator<JavaObject*> > globalRefs;

  /// globalRefsLock - Lock for adding a new global reference.
  ///
  mvm::LockNormal globalRefsLock;
  
  /// appClassLoader - The bootstrap class loader.
  ///
  JnjvmClassLoader* appClassLoader;

  /// hashStr - Hash map of java/lang/String objects allocated by this JVM.
  ///
  StringMap* hashStr;
  
  /// hashUTF8 - Tables of UTF8s defined by this class loader. Shared
  /// by all class loaders in a no isolation configuration.
  ///
  UTF8Map* hashUTF8;

public:
  /// Exceptions - These are the only exceptions VMKit will make.
  ///
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
  void illegalArgumentExceptionForMethod(JavaMethod* meth, UserCommonClass* required,
                                         UserCommonClass* given);
  void illegalArgumentExceptionForField(JavaField* field, UserCommonClass* required,
                                        UserCommonClass* given);
  void illegalArgumentException(const char* msg);
  void classCastException(JavaObject* obj, UserCommonClass* cl);
  void unknownError(const char* fmt, ...); 
  void noSuchFieldError(CommonClass* cl, const UTF8* name);
  void noSuchMethodError(CommonClass* cl, const UTF8* name);
  void classFormatError(const char* fmt, ...);
  void noClassDefFoundError(JavaObject* obj);
  void noClassDefFoundError(const char* fmt, ...);
  void classNotFoundException(JavaString* str);

  /// asciizToStr - Constructs a java/lang/String object from the given asciiz.
  ///
  JavaString* asciizToStr(const char* asciiz);

  /// UTF8ToStr - Constructs a java/lang/String object from the given UTF8.
  ///
  JavaString* UTF8ToStr(const UTF8* utf8);
  
  /// ~Jnjvm - Destroy the JVM.
  ///
  ~Jnjvm();

  /// Jnjvm - Allocate a default JVM, for VT initialization.
  ///
  Jnjvm();

  /// addProperty - Adds a new property in the postProperties map.
  ///
  void addProperty(char* key, char* value);
 
  /// setClasspath - Sets the application classpath for the JVM.
  ///
  void setClasspath(char* cp) {
    classpath = cp;
  }
  
  /// initialiseStatics - Initializes the isolate. The function initialize
  /// static variables in a single environment.
  ///
  ISOLATE_STATIC void initialiseStatics();
  
  ISOLATE_STATIC UserClassPrimitive* getPrimitiveClass(char id) {
    return primitiveMap[id];
  }

  /// Jnjvm - Allocates a new JVM.
  ///
  Jnjvm(mvm::Allocator* allocator);
  
  /// runApplication - Runs the application with the given command line.
  /// User-visible function, inherited by the VirtualMachine class.
  ///
  virtual void runApplication(int argc, char** argv);

};

} // end namespace jnjvm

#endif
