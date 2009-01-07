//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/PrintBuffer.h"
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
  javaThread = thread;
  vmThread = vmth;
  MyVM = isolate;
  interruptFlag = 0;
  state = StateRunning;
  pendingException = 0;
  jniEnv = isolate->jniEnv;
#ifdef SERVICE
  eipIndex = 0;
  replacedEIPs = new void*[100];
  if (isolate->upcalls->newThrowable) {
    ServiceException = isolate->upcalls->newThrowable->doNew(isolate);
  }
#endif
}

JavaThread::~JavaThread() {
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

void JavaThread::startNative(int level) {
  // Call to this function.
  void** cur = (void**)__builtin_frame_address(0);
  
  // Caller (for native Classpath functions).
  //if (level)
  cur = (void**)cur[0];

  // When entering, the number of addresses should be odd.
  // Enable this when finalization gets proper support.
  // assert((addresses.size() % 2) && "Wrong stack");
  
  addresses.push_back(cur);
}

void JavaThread::startJava() {
  // Call to this function.
  void** cur = (void**)__builtin_frame_address(0);
  
  // Caller in JavaMetaJIT.cpp
  cur = (void**)cur[0];

  addresses.push_back(cur);
}

UserClass* JavaThread::getCallingClass(uint32 level) {
  // I'm a native function, so try to look at the last Java method.
  // First Get the caller of this method.
  void** addr = (void**)addresses.back();

  // Get the caller of the Java getCallingClass method.
  if (level)
    addr = (void**)addr[0];
  void* ip = FRAME_IP(addr);

  JavaMethod* meth = getJVM()->IPToMethod<JavaMethod>(ip);

  return meth->classDef;
}

void JavaThread::getJavaFrameContext(std::vector<void*>& context) {
  std::vector<void*>::iterator it = addresses.end();

  // Loop until we cross the first Java frame.
  while (it != addresses.begin()) {
    
    // Get the last Java frame.
    void** addr = (void**)*(--it);
    
    // Set the iterator to the next native -> Java call.
    --it;

    do {
      void* ip = FRAME_IP(addr);
      context.push_back(ip);
      addr = (void**)addr[0];
      // We end walking the stack when we cross a native -> Java call. Here
      // the iterator points to a native -> Java call. We dereference addr twice
      // because a native -> Java call always contains the signature function.
    } while (((void***)addr)[0][0] != *it);
  }
}

void JavaThread::printJavaBacktrace() {
  Jnjvm* vm = getJVM();
  std::vector<void*> vals;
  getJavaFrameContext(vals);
  for (std::vector<void*>::iterator i = vals.begin(), e = vals.end(); 
       i != e; ++i) {
    JavaMethod* meth = vm->IPToMethod<JavaMethod>(*i);
    assert(meth && "Wrong stack");
    fprintf(stderr, "; %p in %s\n",  *i, meth->printString());
  }
}


#include <dlfcn.h>

static void printFunctionInfo(void* ip) {
  Dl_info info;
  int res = dladdr(ip, &info);
  if (res != 0) {
    fprintf(stderr, "; %p in %s\n",  ip, info.dli_sname);
  } else {
    fprintf(stderr, "; %p in Native to Java Frame\n", ip);
  }
}

void JavaThread::printBacktrace() {
  std::vector<void*>::iterator it = addresses.end();
  Jnjvm* vm = getJVM();

  void** addr = (void**)__builtin_frame_address(0);

  // Loop until we cross the first Java frame.
  while (it != addresses.begin()) {
    
    --it;
    // Until we hit the last Java frame.
    while (addr != (void**)*it) {
      void* ip = FRAME_IP(addr);
      printFunctionInfo(ip);
      addr = (void**)addr[0];
    }
    
    // Set the iterator to the next native -> Java call.
    --it;

    do {
      void* ip = FRAME_IP(addr);
      JavaMethod* meth = vm->IPToMethod<JavaMethod>(ip);
      assert(meth && "Wrong stack");
      fprintf(stderr, "; %p in %s\n",  ip, meth->printString());
      addr = (void**)addr[0];
      // End walking the stack when we cross a native -> Java call. Here
      // the iterator points to a native -> Java call. We dereference addr twice
      // because a native -> Java call always contains the signature function.
    } while (((void***)addr)[0][0] != *it);
  }

  while (addr < baseSP && addr < addr[0]) {
    void* ip = FRAME_IP(addr);
    printFunctionInfo(ip);
    addr = (void**)addr[0];
  }

}
