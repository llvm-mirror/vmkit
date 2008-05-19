//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

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

using namespace jnjvm;

const unsigned int JavaThread::StateRunning = 0;
const unsigned int JavaThread::StateWaiting = 1;
const unsigned int JavaThread::StateInterrupted = 2;

void JavaThread::print(mvm::PrintBuffer* buf) const {
  buf->write("Thread:");
  javaThread->print(buf);
}

void JavaThread::destroyer(size_t sz) {
  delete lock;
  delete varcond;
}

JavaThread* JavaThread::get() {
  return (JavaThread*)Thread::threadKey->get();
}

void JavaThread::initialise(JavaObject* thread, Jnjvm* isolate) {
  this->javaThread = thread;
  this->isolate = isolate;
  this->lock = mvm::Lock::allocNormal();
  this->varcond = mvm::Cond::allocCond();
  this->interruptFlag = 0;
  this->state = StateRunning;
  this->self = mvm::Thread::self();
  this->pendingException = 0;
}

JavaObject* JavaThread::currentThread() {
  JavaThread* result = get();
  if (result != 0)
    return result->javaThread;
  else
    return 0;
}

void JavaThread::throwPendingException() {
  JavaThread* th = JavaThread::get();
  assert(th->pendingException);
  void* exc = __cxa_allocate_exception(0);
  th->internalPendingException = exc;
  __cxa_throw(exc, 0, 0);
}

void JavaThread::returnFromNative() {
  assert(sjlj_buffers.size());
#if defined(__MACH__)
  longjmp((int*)sjlj_buffers.back(), 1);
#else
  longjmp((__jmp_buf_tag*)sjlj_buffers.back(), 1);
#endif
}
