//===- ClasspathVMThread.cpp - GNU classpath java/lang/VMThread -----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "types.h"

#include "mvm/Threads/Thread.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMThread_currentThread(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz
#endif
) {
  return (jobject)(JavaThread::currentThread());
}

typedef struct arg_thread_t {
  JavaObject* vmThread;
  JavaThread* intern;
} arg_thread_t;

static void start(arg_thread_t* arg) {
  int argc;
  JavaObject* vmThread = arg->vmThread;
  JavaThread* intern = arg->intern;
  free(arg);
  mvm::Thread::set(intern);
#ifdef MULTIPLE_GC
  intern->GC->inject_my_thread(&argc);
#else
  Collector::inject_my_thread(&argc);
#endif
  UserClass* vmthClass = (UserClass*)vmThread->classOf;
  Jnjvm* isolate = intern->isolate;
  JavaObject* thread = isolate->upcalls->assocThread->getObjectField(vmThread);
  ThreadSystem* ts = isolate->threadSystem;
  bool isDaemon = isolate->upcalls->daemon->getInt8Field(thread);
  intern->threadID = (mvm::Thread::self() << 8) & 0x7FFFFF00;


  if (!isDaemon) {
    ts->nonDaemonLock->lock();
    ts->nonDaemonThreads++;
    ts->nonDaemonLock->unlock();
  }
  
#ifdef SERVICE_VM
  ServiceDomain* vm = (ServiceDomain*)isolate;
  vm->startExecution();
  vm->lock->lock();
  vm->numThreads++;
  vm->lock->unlock();
#endif
  
  isolate->upcalls->runVMThread->invokeIntSpecial(isolate, vmthClass, vmThread);
  
  if (!isDaemon) {
    ts->nonDaemonLock->lock();
    ts->nonDaemonThreads--;
    if (ts->nonDaemonThreads == 0)
      ts->nonDaemonVar->signal();
    ts->nonDaemonLock->unlock();
  }

#ifdef SERVICE_VM
  vm->lock->lock();
  vm->numThreads--;
  vm->lock->unlock();
#endif

#ifdef MULTIPLE_GC
  intern->GC->remove_my_thread();
#else
  Collector::remove_my_thread();
#endif
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_start(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject _vmThread, sint64 stackSize) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* vmThread = (JavaObject*)_vmThread;
  JavaObject* javaThread = vm->upcalls->assocThread->getObjectField(vmThread);
  assert(javaThread);

  JavaThread* th = allocator_new(vm->allocator, JavaThread)();
  th->initialise(javaThread, vm);
  vm->upcalls->vmdataVMThread->setObjectField(vmThread, (JavaObject*)th);
  int tid = 0;
  arg_thread_t* arg = (arg_thread_t*)malloc(sizeof(arg_thread_t));
  arg->intern = th;
  arg->vmThread = vmThread;
#ifdef MULTIPLE_GC
  th->GC = mvm::Thread::get()->GC;
#endif

  mvm::Thread::start(&tid, (int (*)(void *))start, (void*)arg);
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject _vmthread) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* vmthread = (JavaObject*)_vmthread;

  while (vm->upcalls->vmdataVMThread->getObjectField(vmthread) == 0)
    mvm::Thread::yield();
  
  JavaField* field = vm->upcalls->vmdataVMThread;
  JavaThread* th = (JavaThread*)field->getObjectField(vmthread);
  th->lock->lock();
  th->interruptFlag = 1;

  // here we could also raise a signal for interrupting I/O
  if (th->state == JavaThread::StateWaiting) {
    th->state = JavaThread::StateInterrupted;
    th->varcond->signal();
  }
  
  th->lock->unlock();
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_interrupted(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  JavaThread* th = JavaThread::get();
  uint32 interrupt = th->interruptFlag;
  th->interruptFlag = 0;
  return (jboolean)interrupt;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_isInterrupted(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject _vmthread) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* vmthread = (JavaObject*)_vmthread;
  JavaField* field = vm->upcalls->vmdataVMThread;
  JavaThread* th = (JavaThread*)field->getObjectField(vmthread);
  return (jboolean)th->interruptFlag;
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread, jint prio) {
  // Currently not implemented
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread, jobject exc) {
  // Currently not implemented
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  mvm::Thread::yield();
}

}
