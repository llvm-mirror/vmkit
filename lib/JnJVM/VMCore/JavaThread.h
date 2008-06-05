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

extern "C" void* __cxa_allocate_exception(unsigned);
extern "C" void __cxa_throw(void*, void*, void*);

namespace jnjvm {

class Class;
class JavaObject;
class Jnjvm;

class JavaThread : public mvm::Thread {
public:
  static VirtualTable *VT;
  JavaObject* javaThread;
  Jnjvm* isolate;
  mvm::Lock* lock;
  mvm::Cond* varcond;
  JavaObject* pendingException;
  void* internalPendingException;
  uint32 interruptFlag;
  uint32 state;
  std::vector<jmp_buf*> sjlj_buffers;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  virtual void destroyer(size_t sz);
  
  void initialise(JavaObject* thread, Jnjvm* isolate) {
    this->javaThread = thread;
    this->isolate = isolate;
    this->lock = mvm::Lock::allocNormal();
    this->varcond = mvm::Cond::allocCond();
    this->interruptFlag = 0;
    this->state = StateRunning;
    this->pendingException = 0;
  }

  static JavaThread* get() {
    return (JavaThread*)Thread::threadKey->get();
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
  
  static void throwException(JavaObject* obj) {
    JavaThread* th = JavaThread::get();
    assert(th->pendingException == 0 && "pending exception already there?");
    th->pendingException = obj;
    void* exc = __cxa_allocate_exception(0);
    th->internalPendingException = exc;
    __cxa_throw(exc, 0, 0);
  }

  static void throwPendingException() {
    JavaThread* th = JavaThread::get();
    assert(th->pendingException);
    void* exc = __cxa_allocate_exception(0);
    th->internalPendingException = exc;
    __cxa_throw(exc, 0, 0);
  }
  
  static void clearException() {
    JavaThread* th = JavaThread::get();
    th->pendingException = 0;
    th->internalPendingException = 0;
  }

  static bool compareException(Class* cl) {
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
};

} // end namespace jnjvm

#endif
