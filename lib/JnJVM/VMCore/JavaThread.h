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

#include "llvm/PassManager.h"

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
  unsigned int self;
  unsigned int interruptFlag;
  unsigned int state;
  llvm::FunctionPassManager* perFunctionPasses;
  std::vector<jmp_buf*> sjlj_buffers;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void TRACER;
  virtual void destroyer(size_t sz);
  
  void initialise(JavaObject* thread, Jnjvm* isolate);
  static JavaThread* get();
  static JavaObject* currentThread();
  
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

  static void throwPendingException();
  
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

  void returnFromNative();
};

} // end namespace jnjvm

#endif
