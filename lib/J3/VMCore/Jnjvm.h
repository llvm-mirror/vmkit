//===---------- Jnjvm.h - Java virtual machine description ---------------===//
//
//                          The VMKit project
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
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/ObjectLocks.h"

#include "JnjvmConfig.h"
#include "JNIReferences.h"
#include "LockedMap.h"

namespace mvm {
	class MutatorThread;
}

namespace j3 {

class ArrayObject;
class Classpath;
class CommonClass;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaString;
class JavaThread;
class JavaVirtualTable;
class JnjvmBootstrapLoader;
class JnjvmClassLoader;
class ReferenceThread;
class UserClass;
class UserClassArray;
class UserClassPrimitive;
class UserCommonClass;

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

  /// leave - A thread calls this function when it leaves the thread system.
  ///
  void leave();

  /// enter - A thread calls this function when it enters the thread system.
  ///
  void enter();

};

class ClArgumentsInfo {
public:
  int argc;
  char** argv;
  uint32 appArgumentsPos;
  char* className;
  char* jarFile;
  std::vector< std::pair<char*, char*> > agents;

  void readArgs(Jnjvm *vm);
  void extractClassFromJar(Jnjvm* vm, int argc, char** argv, int i);
  void javaAgent(char* cur);

  void printInformation();
  void nyi();
  void printVersion();
};

/// Jnjvm - A JVM. Each execution of a program allocates a Jnjvm.
///
class Jnjvm : public mvm::VirtualMachine {
  friend class JnjvmClassLoader;
public:
  /// throwable - The java/lang/Throwable class. In an isolate
  /// environment, generated code references this field.
  UserClass* throwable;

private:

  virtual size_t getObjectSize(mvm::gc* obj);
  virtual const char* getObjectTypeName(mvm::gc* obj);

  /// CreateError - Creates a Java object of the specified exception class
  /// and calling its <init> function.
  ///
  JavaObject* CreateError(UserClass* cl, JavaMethod* meth, const char* str);
  JavaObject* CreateError(UserClass* cl, JavaMethod* meth, JavaString* str);

  /// error - Throws an exception in the execution of a JVM for the thread
  /// that calls this functions. This is used internally by Jnjvm to control
  /// which pair class/method are used.
  ///
  void error(UserClass* cl, JavaMethod* meth, JavaString*);
  
  /// errorWithExcp - Throws an exception whose cause is the Java object excp.
  ///
  void errorWithExcp(UserClass* cl, JavaMethod* meth, const JavaObject* excp);
  
  /// loadAppClassLoader - Loads the application class loader, so that VMKit
  /// knowns which loader has to load the main class.
  ///
  JnjvmClassLoader* loadAppClassLoader();
  
  /// executeClass - Executes in the given JVM this main class with the given
  /// Java args.
  ///
  void executeClass(const char* className, ArrayObject* args);

  /// executePremain - Executes the premain class for the java/lang/instrument
  /// feature.
  ///
  void executePremain(const char* className, JavaString* args,
                      JavaObject* instrumenter);
   
  /// mainJavaStart - Starts the execution of the application in a Java thread.
  ///
  static void mainJavaStart(mvm::Thread* thread);
  
public:
  
  /// tracer - Traces instances of this class.
  ///
  virtual void tracer(uintptr_t closure);
  
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
 
  /// bootstraLoader - Bootstrap loader for base classes of this virtual
  /// machine.
  ///
  JnjvmBootstrapLoader* bootstrapLoader;

  /// upcalls - Upcalls to call Java methods and access Java fields.
  ///
  Classpath* upcalls;

  /// threadSystem - The thread system to manage non-daemon threads and
  /// control the end of the JVM's execution.
  ///
  ThreadSystem threadSystem;
  
  /// lockSystem - The lock system to allocate and manage Java locks.
  ///
  mvm::LockSystem lockSystem;
  
  /// argumentsInfo - The command line arguments given to the vm
  ///
  ClArgumentsInfo argumentsInfo;

  /// jniEnv - The JNI environment of this JVM.
  ///
  void* jniEnv;

  /// javavmEnv - The Java VM environment of this JVM.
  ///
  const void* javavmEnv;

  /// postProperties - Properties set at runtime and in command line.
  ///
  std::vector< std::pair<char*, char*> > postProperties;

  /// classpath - The CLASSPATH value, or the paths given in command line.
  ///
  const char* classpath;

  /// globalRefs - Global references that JNI wants to protect.
  ///
  JNIGlobalReferences globalRefs;

  /// globalRefsLock - Lock for adding a new global reference.
  ///
  mvm::LockNormal globalRefsLock;
  
  /// appClassLoader - The bootstrap class loader.
  ///
  JnjvmClassLoader* appClassLoader;

  /// hashStr - Hash map of java/lang/String objects allocated by this JVM.
  ///
  StringMap hashStr;

	/// javaMainThread - the java main thread
	JavaThread* javaMainThread;

	mvm::VirtualTable* VMClassLoader__VT;

	void initialiseInternalVTs();
public:
  
  /// CreateExceptions - These are the runtime exceptions thrown by Java code
  /// compiled by VMKit.
  ///
  JavaObject* CreateNullPointerException();
  JavaObject* CreateOutOfMemoryError();
  JavaObject* CreateIndexOutOfBoundsException(sint32 entry);
  JavaObject* CreateNegativeArraySizeException();
  JavaObject* CreateClassCastException(JavaObject* obj, UserCommonClass* cl);
  JavaObject* CreateArithmeticException();
  JavaObject* CreateStackOverflowError();
  JavaObject* CreateLinkageError(const char* msg);
  JavaObject* CreateArrayStoreException(JavaVirtualTable* VT);
  JavaObject* CreateUnsatisfiedLinkError(JavaMethod* meth);
  
  /// Exceptions - These are the only exceptions VMKit will make.
  ///
  void arrayStoreException();
  void indexOutOfBounds(const JavaObject* obj, sint32 entry);
  void negativeArraySizeException(int size);
  void nullPointerException();
  void illegalAccessException(const char* msg);
  void illegalMonitorStateException(const JavaObject* obj);
  void interruptedException(const JavaObject* obj);
  void initializerError(const JavaObject* excp);
  void invocationTargetException(const JavaObject* obj);
  void outOfMemoryError();
  void noClassDefFoundError(JavaObject* obj);
  void instantiationException(UserCommonClass* cl);
  void instantiationError(UserCommonClass* cl);
  void illegalArgumentException(const char* msg);
  void classCastException(JavaObject* obj, UserCommonClass* cl);
  void noSuchFieldError(CommonClass* cl, const UTF8* name);
  void noSuchMethodError(CommonClass* cl, const UTF8* name); 
  void abstractMethodError(CommonClass* cl, const UTF8* name); 
  void noClassDefFoundError(const UTF8* name);
  void classNotFoundException(JavaString* str);
  void noClassDefFoundError(UserClass* cl, const UTF8* name);
  void classFormatError(const char* str);
  
  /// asciizToStr - Constructs a java/lang/String object from the given asciiz.
  ///
  JavaString* asciizToStr(const char* asciiz);

  /// UTF8ToStr - Constructs a java/lang/String object from the given UTF8.
  ///
  JavaString* constructString(const ArrayUInt16* array);
  
  /// UTF8ToStr - Constructs a java/lang/String object from the given internal
  /// UTF8, thus duplicating the UTF8.
  ///
  JavaString* internalUTF8ToStr(const UTF8* utf8);
     
  /// asciizToUTF8 - Constructs an UTF8 out of the asciiz.
  ///
  ArrayUInt16* asciizToArray(const char* asciiz);

  /// finalizeObject - invoke the finalizer of a java object
  ///
	virtual void finalizeObject(mvm::gc* obj);
  
  /// getReferentPtr - return the referent of a reference
  ///
	virtual mvm::gc** getReferent(mvm::gc* ref);

  /// setReferentPtr - set the referent of a reference
  ///
	virtual void setReferent(mvm::gc* ref, mvm::gc* val);

  /// enqueueReference - enqueue the reference
  ///
	virtual bool enqueueReference(mvm::gc* _obj);

  /// ~Jnjvm - Destroy the JVM.
  ///
  ~Jnjvm();

  /// addProperty - Adds a new property in the postProperties map.
  ///
  void addProperty(char* key, char* value);
 
  /// setClasspath - Sets the application classpath for the JVM.
  ///
  void setClasspath(char* cp) {
    classpath = cp;
  }

  /// Jnjvm - Allocates a new JVM.
  ///
  Jnjvm(mvm::BumpPtrAllocator& Alloc, mvm::VMKit* vmkit, JavaCompiler* Comp, bool dlLoad);
  
  /// runApplication - Runs the application with the given command line.
  /// User-visible function, inherited by the VirtualMachine class.
  ///
  virtual void runApplication(int argc, char** argv);

  /// waitForExit - Waits that there are no more non-daemon threads in this JVM.
  ///
  virtual void waitForExit();

	/// buildVMThreadData - allocate a java thread for the underlying mutator. Called when the java thread is a foreign thread.
	///
	virtual mvm::VMThreadData* buildVMThreadData(mvm::Thread* mut);

  /// loadBootstrap - Bootstraps the JVM, getting the class loader, initializing
  /// bootstrap classes (e.g. java/lang/Class, java/lang/Exception) and
  /// mapping the initial thread.
  ///
  void loadBootstrap();

	// asjavaException - convert from gc to JavaObject. Will be used to identify the points
	// where we must test the original vm of the exception
	static JavaObject* asJavaException(mvm::gc* o) { llvm_gcroot(o, 0); return (JavaObject*)o; } 
};

} // end namespace j3

#endif
