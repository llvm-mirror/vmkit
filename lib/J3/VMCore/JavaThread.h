//===----------- JavaThread.h - Java thread description -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_THREAD_H
#define JNJVM_JAVA_THREAD_H

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/ObjectLocks.h"
#include "mvm/Threads/Thread.h"

#include "MutatorThread.h"

#include "JavaObject.h"
#include "JNIReferences.h"

namespace j3 {

class Class;
class JavaMethod;
class JavaObject;
class Jnjvm;


#define BEGIN_NATIVE_EXCEPTION(level)						\
  JavaThread* __th = JavaThread::get();					\
  TRY {

#define END_NATIVE_EXCEPTION										\
  } CATCH {																			\
    __th->throwFromNative();										\
  } END_CATCH;

#define BEGIN_JNI_EXCEPTION														\
  mvm::Thread* mut = mvm::Thread::get();							\
  void* SP = mut->getLastSP();												\
  mut->leaveUncooperativeCode();											\
  mvm::KnownFrame Frame;															\
  mut->startKnownFrame(Frame);												\
  TRY {

#define END_JNI_EXCEPTION													\
  } CATCH {																				\
		JavaThread::j3Thread(mut)->throwFromJNI(SP);	\
  } END_CATCH;

#define RETURN_FROM_JNI(a) {											\
		mut->endKnownFrame();													\
		mut->enterUncooperativeCode(SP);							\
		return (a); }																	\

#define RETURN_VOID_FROM_JNI {										\
		mut->endKnownFrame();													\
		mut->enterUncooperativeCode(SP);							\
		return; }																			\

/// JavaThread - This class is the internal representation of a Java thread.
/// It maintains thread-specific information such as its state, the current
/// exception if there is one, the layout of the stack, etc.
///
class JavaThread : public mvm::VMThreadData {
public:
  
  /// jniEnv - The JNI environment of the thread.
  ///
  void* jniEnv;

  /// javaThread - The Java representation of this thread.
  ///
  JavaObject* javaThread;

  /// vmThread - The VMThread object of this thread.
  ///
  JavaObject* vmThread;

  mvm::LockingThread lockingThread;
  
  /// currentAddedReferences - Current number of added local references.
  ///
  uint32_t* currentAddedReferences;

  /// localJNIRefs - List of local JNI references.
  ///
  JNILocalReferences* localJNIRefs;

	mvm::gc** pushJNIRef(mvm::gc* obj) {
    llvm_gcroot(obj, 0);
    if (!obj) return 0;
   
    ++(*currentAddedReferences);
    return localJNIRefs->addJNIReference(this, obj);

  }

  /// tracer - Traces GC-objects pointed by this thread object.
  ///
  virtual void tracer(uintptr_t closure);

  /// JavaThread - Empty constructor, used to get the VT.
  ///
  JavaThread() : mvm::VMThreadData(0, 0) {
  }

  /// ~JavaThread - Delete any potential malloc'ed objects used by this thread.
  ///
  ~JavaThread();

  /// JavaThread - Creates a Java thread. 
  ///
  JavaThread(Jnjvm* vm, mvm::Thread*mut);

  /// create - Creates a Java thread and a mutator thread.
  ///
	static JavaThread* create(Jnjvm* vm);

  /// j3Thread - gives the JavaThread associated with the mutator thread
  ///
	static JavaThread*  j3Thread(mvm::Thread* mut);

  /// initialise - initialise the thread
  ///
  void initialise(JavaObject* thread, JavaObject* vmth);
  
  /// get - Get the current thread as a J3 object.
  ///
  static JavaThread* get() {
    return j3Thread(mvm::Thread::get());
  }

  /// getJVM - Get the Java VM in which this thread executes.
  ///
  Jnjvm* getJVM() {
    return (Jnjvm*)vm;
  }

  /// currentThread - Return the current thread as a Java object.
  ///
  JavaObject* currentThread() {
    return javaThread;
  }

  /// throwFromJNI - Throw an exception after executing JNI code.
  ///
  void throwFromJNI(void* SP) {
    mut->endKnownFrame();
    mut->enterUncooperativeCode(SP);
  }
  
  /// throwFromNative - Throw an exception after executing Native code.
  ///
  void throwFromNative() {
#ifdef DWARF_EXCEPTIONS
    mut->throwIt();
#endif
  }
  
  /// throwFromJava - Throw an exception after executing Java code.
  ///
  void throwFromJava() {
    mut->throwIt();
  }

  /// startJava - Interesting, but actually does nothing :)
  void startJava() {}
  
  /// endJava - Interesting, but actually does nothing :)
  void endJava() {}

  /// startJNI - Record that we are entering native code.
  ///
  void startJNI();

  void endJNI();

  /// getCallingMethod - Get the Java method in the stack at the specified
  /// level.
  ///
  JavaMethod* getCallingMethodLevel(uint32 level);
  
  /// getCallingClassLevel - Get the Java method in the stack at the
  /// specified level.
  ///
  UserClass* getCallingClassLevel(uint32 level);
  
  /// getNonNullClassLoader - Get the first non-null class loader on the
  /// stack.
  ///
  JavaObject* getNonNullClassLoader();
    
  /// printJavaBacktrace - Prints the backtrace of this thread. Only prints
  /// the Java methods on the stack.
  ///
  void printJavaBacktrace();

  /// getJavaFrameContext - Fill the buffer with Java methods currently on
  /// the stack.
  ///
  uint32 getJavaFrameContext(void** buffer);
};

} // end namespace j3

#endif
