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
  if (javaThread) javaThread->print(buf);
}

JavaThread::JavaThread(JavaObject* thread, JavaObject* vmth, Jnjvm* isolate) {
  javaThread = thread;
  vmThread = vmth;
  MyVM = isolate;
  interruptFlag = 0;
  state = StateRunning;
  pendingException = 0;
}

JavaThread::~JavaThread() {}

// We define these here because gcc compiles the 'throw' keyword
// differently, whether these are defined in a file or not. Since many
// cpp files import JavaThread.h, they couldn't use the keyword.

extern "C" void* __cxa_allocate_exception(unsigned);
extern "C" void __cxa_throw(void*, void*, void*);

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
