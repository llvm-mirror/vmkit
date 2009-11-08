//===----------- JavaThread.h - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_THREAD_H
#define JNJVM_JAVA_THREAD_H

#include <csetjmp>
#include <vector>

#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "MutatorThread.h"

#include "JavaObject.h"
#include "JNIReferences.h"

namespace jnjvm {

class Class;
class JavaMethod;
class JavaObject;
class Jnjvm;


#define BEGIN_NATIVE_EXCEPTION(level) \
  JavaThread* __th = JavaThread::get(); \
  __th->startNative(level); \
  try {

#define END_NATIVE_EXCEPTION \
  } catch(...) { \
    __th->throwFromNative(); \
  } \
  __th->endNative();

#define BEGIN_JNI_EXCEPTION \
  JavaThread* th = JavaThread::get(); \
  void* SP = th->getLastSP(); \
  th->leaveUncooperativeCode(); \
  th->addresses.push_back(0); \
  th->startNative(0); \
  mvm::KnownFrame Frame; \
  Frame.previousFrame = th->lastKnownFrame; \
  Frame.currentFP = th->addresses.back(); \
  th->lastKnownFrame = &Frame; \
  try {

#define END_JNI_EXCEPTION \
  } catch(...) { \
    th->lastKnownFrame = th->lastKnownFrame->previousFrame; \
    th->endNative(); \
    th->addresses.pop_back(); \
    th->enterUncooperativeCode(SP); \
    th->throwFromJNI(); \
  }

#define RETURN_FROM_JNI(a) {\
  th->lastKnownFrame = th->lastKnownFrame->previousFrame; \
  th->endNative(); \
  th->addresses.pop_back(); \
  th->enterUncooperativeCode(SP); \
  return (a); } \

#define RETURN_VOID_FROM_JNI {\
  th->lastKnownFrame = th->lastKnownFrame->previousFrame; \
  th->endNative(); \
  th->addresses.pop_back(); \
  th->enterUncooperativeCode(SP); \
  return; } \


/// JavaThread - This class is the internal representation of a Java thread.
/// It maintains thread-specific information such as its state, the current
/// exception if there is one, the layout of the stack, etc.
///
class JavaThread : public mvm::MutatorThread {
public:
  
  /// jniEnv - The JNI environment of the thread.
  ///
  void* jniEnv;
  
  /// pendingException - The Java exception currently pending.
  ///
  JavaObject* pendingException;

  /// internalPendingException - The C++ exception currencty pending.
  ///
  void* internalPendingException;

  /// javaThread - The Java representation of this thread.
  ///
  JavaObject* javaThread;

  /// vmThread - The VMThread object of this thread.
  ///
  JavaObject* vmThread;

  /// varcond - Condition variable when the thread needs to be awaken from
  /// a wait.
  ///
  mvm::Cond varcond;

  /// interruptFlag - Has this thread been interrupted?
  ///
  uint32 interruptFlag;

  /// nextWaiting - Next thread waiting on the same monitor.
  ///
  JavaThread* nextWaiting;
  
  /// prevWaiting - Previous thread waiting on the same monitor.
  ///
  JavaThread* prevWaiting;

  /// waitsOn - The monitor on which the thread is waiting on.
  ///
  JavaLock* waitsOn;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  /// state - The current state of this thread: Running, Waiting or Interrupted.
  uint32 state;
  
  /// currentSjljBuffers - Current buffer pushed when entering a non-JVM native
  /// function and popped when leaving the function. The buffer is used when
  /// the native function throws an exception through a JNI throwException call.
  ///
  void* currentSjljBuffer;

  /// currentAddedReferences - Current number of added local references.
  ///
  uint32_t* currentAddedReferences;

  /// localJNIRefs - List of local JNI references.
  ///
  JNILocalReferences* localJNIRefs;


  JavaObject** pushJNIRef(JavaObject* obj) {
    if (!obj) return 0;
   
    ++(*currentAddedReferences);
    return localJNIRefs->addJNIReference(this, obj);

  }

  /// tracer - Traces GC-objects pointed by this thread object.
  ///
  virtual void tracer();

  /// JavaThread - Empty constructor, used to get the VT.
  ///
  JavaThread() {
#ifdef SERVICE
    replacedEIPs = 0;
#endif
  }

  /// ~JavaThread - Delete any potential malloc'ed objects used by this thread.
  ///
  ~JavaThread();
  
  /// JavaThread - Creates a Java thread.
  ///
  JavaThread(JavaObject* thread, JavaObject* vmThread, Jnjvm* isolate);

  /// get - Get the current thread as a JnJVM object.
  ///
  static JavaThread* get() {
    return (JavaThread*)mvm::Thread::get();
  }

  /// getJVM - Get the JnJVM in which this thread executes.
  ///
  Jnjvm* getJVM() {
    return (Jnjvm*)MyVM;
  }

  /// currentThread - Return the current thread as a Java object.
  ///
  JavaObject* currentThread() {
    return javaThread;
  }
 
  /// throwException - Throw the given exception in the current thread.
  ///
  void throwException(JavaObject* obj);

  /// throwPendingException - Throw a pending exception.
  ///
  void throwPendingException();
  
  /// getJavaException - Return the pending exception.
  ///
  JavaObject* getJavaException() {
    return pendingException;
  }

  /// throwFromJNI - Throw an exception after executing JNI code.
  ///
  void throwFromJNI() {
    assert(currentSjljBuffer);
    internalPendingException = 0;
#if defined(__MACH__)
    longjmp((int*)currentSjljBuffer, 1);
#else
    longjmp((__jmp_buf_tag*)currentSjljBuffer, 1);
#endif
  }
  
  /// throwFromNative - Throw an exception after executing Native code.
  ///
  void throwFromNative() {
#ifdef DWARF_EXCEPTIONS
    addresses.pop_back();
    throwPendingException();
#endif
  }
  
  /// throwFromJava - Throw an exception after executing Java code.
  ///
  void throwFromJava() {
    addresses.pop_back();
    throwPendingException();
  }
  
  /// startJNI - Record that we are entering native code.
  ///
  void startJNI(int level) __attribute__ ((noinline));

  /// startJava - Record that we are entering Java code.
  ///
  void startJava() __attribute__ ((noinline));
  
  void endJNI() {
    assert(!(addresses.size() % 2) && "Wrong stack");    
  
    localJNIRefs->removeJNIReferences(this, *currentAddedReferences);
   
    // Go back to cooperative mode.
    leaveUncooperativeCode();
   
    // Pop the address after calling leaveUncooperativeCode
    // to let the thread's call stack coherent.
    addresses.pop_back();
    lastKnownFrame = lastKnownFrame->previousFrame;
  }

  /// endJava - Record that we are leaving Java code.
  ///
  void endJava() {
    assert((addresses.size() % 2) && "Wrong stack");    
    addresses.pop_back();
  }

  bool isInNative() {
    return !(addresses.size() % 2);
  }
  
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

  /// getJavaFrameContext - Fill the vector with Java frames
  /// currently on the stack.
  ///
  void getJavaFrameContext(std::vector<void*>& context);

private:
  /// internalClearException - Clear the C++ and Java exceptions
  /// currently pending.
  ///
  virtual void internalClearException() {
    pendingException = 0;
    internalPendingException = 0;
  }

public:

#ifdef SERVICE
  /// ServiceException - The exception that will be thrown if a bundle is
  /// stopped.
  JavaObject* ServiceException;

  /// replacedEIPs - List of instruction pointers which must be replaced
  /// to a function that throws an exception. We maintain this list and update
  /// the stack correctly so that Dwarf unwinding does not complain.
  ///
  void** replacedEIPs;

  /// eipIndex - The current index in the replacedIPs list.
  ///
  uint32_t eipIndex;
#endif

};

} // end namespace jnjvm

#endif
