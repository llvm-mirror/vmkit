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

#include <setjmp.h>

#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Key.h"
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
  Jnjvm* isolate;
  mvm::LockNormal lock;
  mvm::Cond varcond;
  JavaObject* pendingException;
  void* internalPendingException;
  uint32 interruptFlag;
  uint32 state;
  std::vector<jmp_buf*> sjlj_buffers;
  bool bootstrap;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  JavaThread() { bootstrap = true; }
  ~JavaThread();
  
  JavaThread(JavaObject* thread, Jnjvm* isolate, void* sp);

  static JavaThread* get() {
    return (JavaThread*)mvm::Thread::get();
  }
  static JavaObject* currentThread() {
    JavaThread* result = get();
    if (result != 0)
      return result->javaThread;
    else
      return 0;
  }
  
  static void* getException() {
    return (void*)
      ((char*)JavaThread::get()->internalPendingException - 8 * sizeof(void*));
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

  void returnFromNative() {
    assert(sjlj_buffers.size());
#if defined(__MACH__)
    longjmp((int*)sjlj_buffers.back(), 1);
#else
    longjmp((__jmp_buf_tag*)sjlj_buffers.back(), 1);
#endif
  }
  
private:
  virtual void internalClearException() {
    pendingException = 0;
    internalPendingException = 0;
  }
};

} // end namespace jnjvm

#endif
