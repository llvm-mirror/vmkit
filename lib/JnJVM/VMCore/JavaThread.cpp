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
  // Caller of this function.
  void** cur = (void**)FRAME_PTR();
  unsigned level2 = level;
  
  while (level--)
    cur = (void**)cur[0];

  // When entering, the number of addresses should be odd.
  assert((addresses.size() % 2) && "Wrong stack");
  
  addresses.push_back(cur);
  lastKnownFrame->currentFP = cur;

  // Start uncooperative mode.
  enterUncooperativeCode(level2);
}

void JavaThread::startJava() {
  // Caller of this function.
  void** cur = (void**)FRAME_PTR();
  cur = (void**)cur[0];
  
  assert(!(addresses.size() % 2) && "Wrong stack");
  
  addresses.push_back(cur);
}


UserClass* JavaThread::getCallingClass(uint32 level) {
  // I'm a native function, so try to look at the last Java method.
  // First take the getCallingClass address.
  void** addr = (void**)addresses.back();
  
  // Caller of getCallingClass.
  addr = (void**)addr[0];

  // Get the caller of the caller of the Java getCallingClass method.
  if (level)
    addr = (void**)addr[0];
  void* ip = FRAME_IP(addr);

  mvm::MethodInfo* MI = getJVM()->IPToMethodInfo(ip);
  JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();

  return meth->classDef;
}

JavaMethod* JavaThread::getCallingMethod() {
  mvm::StackWalker Walker(this);

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1) {
      JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();
      return meth;
    }
    ++Walker;
  }
  return 0;
}
  
UserClass* JavaThread::getCallingClassFromJNI() {
  JavaMethod* meth = getCallingMethod();
  if (meth) return meth->classDef;
  return 0;
}

void JavaThread::getJavaFrameContext(std::vector<void*>& context) {
  std::vector<void*>::iterator it = addresses.end();

  // Loop until we cross the first Java frame.
  while (it != addresses.begin()) {
    
    // Get the last Java frame.
    void** addr = (void**)*(--it);
    
    // Set the iterator to the next native -> Java call.
    --it;
    
    // See if we're from JNI.
    if (*it == 0) {
      --it;
      addr = (void**)*it;
      --it;
      if (*it == 0) {
        addr = (void**)addr[0];
        continue;
      }
    }

    do {
      void* ip = FRAME_IP(addr);
      bool isStub = ((unsigned char*)ip)[0] == 0xCE;
      if (isStub) ip = addr[2];
      context.push_back(ip);
      addr = (void**)addr[0];
      // We end walking the stack when we cross a native -> Java call. Here
      // the iterator points to a native -> Java call. We dereference addr twice
      // because a native -> Java call always contains the signature function.
    } while (((void***)addr)[0][0] != *it);
  }
}

UserClass* JavaThread::getCallingClassLevel(uint32 level) {
  mvm::StackWalker Walker(this);
  uint32 index = 0;

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->MethodType == 1) {
      if (index == level) {
        JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();
        return meth->classDef;
      }
      ++index;
    }
    ++Walker;
  }
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
