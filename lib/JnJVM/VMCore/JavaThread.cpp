//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"


using namespace jnjvm;

const unsigned int JavaThread::StateRunning = 0;
const unsigned int JavaThread::StateWaiting = 1;
const unsigned int JavaThread::StateInterrupted = 2;

JavaThread::JavaThread(JavaObject* thread, JavaObject* vmth, Jnjvm* isolate) {
  llvm_gcroot(thread, 0);
  llvm_gcroot(vmth, 0);
  
  javaThread = thread;
  vmThread = vmth;
  MyVM = isolate;
  interruptFlag = 0;
  state = StateRunning;
  pendingException = 0;
  internalPendingException = 0;
  lastKnownFrame = 0;
  jniEnv = isolate->jniEnv;
  localJNIRefs = new JNILocalReferences();
  currentAddedReferences = 0;
  currentSjljBuffer = 0;

#ifdef SERVICE
  eipIndex = 0;
  replacedEIPs = new void*[100];
  if (isolate->upcalls->newThrowable) {
    ServiceException = isolate->upcalls->newThrowable->doNew(isolate);
  }
#endif
}

JavaThread::~JavaThread() {
  delete localJNIRefs;
#ifdef SERVICE
  delete replacedEIPs;
#endif
}

// We define these here because gcc compiles the 'throw' keyword
// differently, whether these are defined in a file or not. Since many
// cpp files import JavaThread.h, they couldn't use the keyword.

extern "C" void* __cxa_allocate_exception(unsigned);
extern "C" void __cxa_throw(void*, void*, void*);

void JavaThread::throwException(JavaObject* obj) {
  llvm_gcroot(obj, 0);
  JavaThread* th = JavaThread::get();
  assert(th->pendingException == 0 && "pending exception already there?");
  th->pendingException = obj;
  void* exc = __cxa_allocate_exception(0);
  // 32 = sizeof(_Unwind_Exception) in libgcc...  
  th->internalPendingException = (void*)((uintptr_t)exc - 32);
  __cxa_throw(exc, 0, 0);
}

void JavaThread::throwPendingException() {
  JavaThread* th = JavaThread::get();
  assert(th->pendingException);
  void* exc = __cxa_allocate_exception(0);
  th->internalPendingException = (void*)((uintptr_t)exc - 32);
  __cxa_throw(exc, 0, 0);
}

void JavaThread::startJNI(int level) {
  // Start uncooperative mode.
  enterUncooperativeCode(level);
}

uint32 JavaThread::getJavaFrameContext(void** buffer) {
  mvm::StackWalker Walker(this);
  uint32 i = 0;

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1) {
      JavaMethod* M = (JavaMethod*)MI->getMetaInfo();
      buffer[i++] = M;
    }
    ++Walker;
  }
  return i;
}

JavaMethod* JavaThread::getCallingMethodLevel(uint32 level) {
  mvm::StackWalker Walker(this);
  uint32 index = 0;

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1) {
      if (index == level) {
        return (JavaMethod*)MI->getMetaInfo();
      }
      ++index;
    }
    ++Walker;
  }
  return 0;
}

UserClass* JavaThread::getCallingClassLevel(uint32 level) {
  JavaMethod* meth = getCallingMethodLevel(level);
  if (meth) return meth->classDef;
  return 0;
}

JavaObject* JavaThread::getNonNullClassLoader() {
  
  JavaObject* obj = 0;
  llvm_gcroot(obj, 0);
  
  mvm::StackWalker Walker(this);

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1) {
      JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();
      JnjvmClassLoader* loader = meth->classDef->classLoader;
      obj = loader->getJavaClassLoader();
      if (obj) return obj;
    }
    ++Walker;
  }
  return 0;
}


void JavaThread::printJavaBacktrace() {
  mvm::StackWalker Walker(this);

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1)
      MI->print(Walker.ip, Walker.addr);
    ++Walker;
  }
}

JavaObject** JNILocalReferences::addJNIReference(JavaThread* th,
                                                 JavaObject* obj) {
  llvm_gcroot(obj, 0);
  
  if (length == MAXIMUM_REFERENCES) {
    JNILocalReferences* next = new JNILocalReferences();
    th->localJNIRefs = next;
    next->prev = this;
    return next->addJNIReference(th, obj);
  } else {
    localReferences[length] = obj;
    return &localReferences[length++];
  }
}

void JNILocalReferences::removeJNIReferences(JavaThread* th, uint32_t num) {
    
  if (th->localJNIRefs != this) {
    delete th->localJNIRefs;
    th->localJNIRefs = this;
  }

  if (num > length) {
    assert(prev && "No prev and deleting too much local references");
    prev->removeJNIReferences(th, num - length);
  } else {
    length -= num;
  }
}
