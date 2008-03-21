//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Key.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"

using namespace jnjvm;

const unsigned int JavaThread::StateRunning = 0;
const unsigned int JavaThread::StateWaiting = 1;
const unsigned int JavaThread::StateInterrupted = 2;

void JavaThread::print(mvm::PrintBuffer* buf) const {
  buf->write("Thread:");
  javaThread->print(buf);
}
 
void JavaThread::destroyer(size_t sz) {
  delete perFunctionPasses;
}

JavaThread* JavaThread::get() {
  return threadKey->get();
}

extern void AddStandardCompilePasses(llvm::FunctionPassManager*);

JavaThread* JavaThread::allocate(JavaObject* thread, Jnjvm* isolate) {
  JavaThread* key = gc_new(JavaThread)();
  key->javaThread = thread;
  key->isolate = isolate;
  key->lock = mvm::Lock::allocNormal();
  key->varcond = mvm::Cond::allocCond();
  key->interruptFlag = 0;
  key->state = StateRunning;
  key->self = mvm::Thread::self();
  key->pendingException = 0;
  key->perFunctionPasses = new llvm::FunctionPassManager(isolate->TheModuleProvider);
  key->perFunctionPasses->add(new llvm::TargetData(isolate->module)); 
  AddStandardCompilePasses(key->perFunctionPasses);
  return key;
}

JavaObject* JavaThread::currentThread() {
  JavaThread* result = get();
  if (result != 0)
    return result->javaThread;
  else
    return 0;
}

extern "C" void* __cxa_allocate_exception(unsigned);
extern "C" void __cxa_throw(void*, void*, void*);

void* JavaThread::getException() {
  return (void*)((char*)JavaThread::get()->internalPendingException - 8 * sizeof(void*));
}

JavaObject* JavaThread::getJavaException() {
  return JavaThread::get()->pendingException;
}


void JavaThread::throwException(JavaObject* obj) {
  JavaThread* th = JavaThread::get();
  assert(th->pendingException == 0 && "pending exception already there?");
  th->pendingException = obj;
  void* exc = __cxa_allocate_exception(0);
  th->internalPendingException = exc;
  __cxa_throw(exc, 0, 0);
}

void JavaThread::throwPendingException() {
  JavaThread* th = JavaThread::get();
  assert(th->pendingException);
  void* exc = __cxa_allocate_exception(0);
  th->internalPendingException = exc;
  __cxa_throw(exc, 0, 0);
}

void JavaThread::clearException() {
  JavaThread* th = JavaThread::get();
  th->pendingException = 0;
  th->internalPendingException = 0;
}

bool JavaThread::compareException(Class* cl) {
  JavaObject* pe = JavaThread::get()->pendingException;
  assert(pe && "no pending exception?");
  bool val = pe->classOf->subclassOf(cl);
  return val;
}

void JavaThread::returnFromNative() {
  assert(sjlj_buffers.size());
  longjmp((__jmp_buf_tag*)sjlj_buffers.back(), 1);
}
