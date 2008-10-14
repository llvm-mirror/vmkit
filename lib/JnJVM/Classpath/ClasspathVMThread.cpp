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

static void start(JavaObject* vmThread) {
  // Get the classpath
  Classpath* upcalls = vmThread->classOf->classLoader->bootstrapLoader->upcalls;

  // When starting the thread, the creator placed the vm object in the vmThread
  Jnjvm* vm = (Jnjvm*)upcalls->vmdataVMThread->getObjectField(vmThread);
  assert(vm && "Thread Creator did not put the vm");
 
  // Classpath has set this field.
  JavaObject* javaThread = upcalls->assocThread->getObjectField(vmThread);
  assert(javaThread && "VMThread with no Java equivalent");
  
  // Create the Java thread.Allocating it on stack will make it call the
  // destructor when the function exists.
  JavaThread th(javaThread, vm, &upcalls);

  // Ok, now we can set the real value of vmdata, which is the JavaThread
  // object.
  vm->upcalls->vmdataVMThread->setObjectField(vmThread, (JavaObject*)(void*)&th);

  UserClass* vmthClass = (UserClass*)vmThread->classOf;
  ThreadSystem& ts = vm->threadSystem;
  
  
  bool isDaemon = vm->upcalls->daemon->getInt8Field(javaThread);

  if (!isDaemon) {
    ts.nonDaemonLock.lock();
    ts.nonDaemonThreads++;
    ts.nonDaemonLock.unlock();
  }
  
  upcalls->runVMThread->invokeIntSpecial(vm, vmthClass, vmThread);
  
  if (!isDaemon) {
    ts.nonDaemonLock.lock();
    ts.nonDaemonThreads--;
    if (ts.nonDaemonThreads == 0)
      ts.nonDaemonVar.signal();
    ts.nonDaemonLock.unlock();
  }
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_start(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject _vmThread, sint64 stackSize) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* vmThread = (JavaObject*)_vmThread;
  
  // Set the vm in the vmData. We do this to give the vm to the created
  // thread. The created thread will change the field to its JavaThread
  // object.
  vm->upcalls->vmdataVMThread->setObjectField(vmThread, (JavaObject*)vm);

  int tid = 0;

  mvm::Thread::start(&tid, (int (*)(void *))start, (void*)vmThread);
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject _vmthread) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* vmthread = (JavaObject*)_vmthread;
  JavaField* field = vm->upcalls->vmdataVMThread; 
  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  while (field->getObjectField(vmthread) == 0)
    mvm::Thread::yield();
  
  JavaThread* th = (JavaThread*)field->getObjectField(vmthread);
  th->lock.lock();
  th->interruptFlag = 1;

  // here we could also raise a signal for interrupting I/O
  if (th->state == JavaThread::StateWaiting) {
    th->state = JavaThread::StateInterrupted;
    th->varcond.signal();
  }
  
  th->lock.unlock();
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
