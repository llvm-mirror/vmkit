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

#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "JavaObject.h"

namespace jnjvm {

class Class;
class JavaObject;
class Jnjvm;

class JavaThread : public mvm::Thread {
public:
  static VirtualTable *VT;
  JavaObject* javaThread;
  JavaObject* vmThread;
  mvm::LockNormal lock;
  mvm::Cond varcond;
  JavaObject* pendingException;
  void* internalPendingException;
  uint32 interruptFlag;
  uint32 state;
  std::vector<jmp_buf*> sjlj_buffers;
  std::vector<void*> addresses;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  JavaThread() {
#ifdef SERVICE
    replacedEIPs = 0;
#endif
  }
  ~JavaThread();
  
  JavaThread(JavaObject* thread, JavaObject* vmThread, Jnjvm* isolate);

  static JavaThread* get() {
    return (JavaThread*)mvm::Thread::get();
  }

  Jnjvm* getJVM() {
    return (Jnjvm*)MyVM;
  }

  static JavaObject* currentThread() {
    JavaThread* result = get();
    if (result != 0)
      return result->javaThread;
    else
      return 0;
  }
  
  static void* getException() {
    // 32 = sizeof(_Unwind_Exception) in libgcc...
    return (void*)
      ((uintptr_t)JavaThread::get()->internalPendingException - 32);
  }
 
  /// throwException - Throws the given exception in the current thread.
  ///
  static void throwException(JavaObject* obj);

  /// throwPendingException - Throws a pending exception created by JNI.
  ///
  static void throwPendingException();
  
  static bool compareException(UserClass* cl) {
    JavaObject* pe = JavaThread::get()->pendingException;
    assert(pe && "no pending exception?");
    bool val = pe->classOf->subclassOf(cl);
    return val;
  }
  
  static JavaObject* getJavaException() {
    return JavaThread::get()->pendingException;
  }

  void returnFromJNI() {
    assert(sjlj_buffers.size());
#if defined(__MACH__)
    longjmp((int*)sjlj_buffers.back(), 1);
#else
    longjmp((__jmp_buf_tag*)sjlj_buffers.back(), 1);
#endif
  }
  
  void returnFromNative() {
    addresses.pop_back();
    throwPendingException();
  }
  
  void returnFromJava() {
    addresses.pop_back();
    throwPendingException();
  }

  void startNative(int level);
  void startJava();
  
  void endNative() {
    addresses.pop_back();
  }

  void endJava() {
    addresses.pop_back();
  }

  /// getCallingClass - Get the Java method that called the last Java
  /// method on the stack.
  ///
  UserClass* getCallingClass();
    
  /// printBacktrace - Prints the backtrace of this thread.
  ///
  void printBacktrace();

  /// getJavaFrameContext - Fill the vector with Java frames
  /// currently on the stack.
  void getJavaFrameContext(std::vector<void*>& context);

  /// printString - Prints the class.
  char *printString() const {
    mvm::PrintBuffer *buf = mvm::PrintBuffer::alloc();
    print(buf);
    return buf->contents()->cString();
  }

#ifdef SERVICE
  JavaObject* ServiceException;
  void** replacedEIPs;
  uint32_t eipIndex;
#endif

  static bool isJavaThread(mvm::Thread* th) {
    return ((void**)th)[0] == VT;
  }
  
private:
  virtual void internalClearException() {
    pendingException = 0;
    internalPendingException = 0;
  }
};

} // end namespace jnjvm

#endif
