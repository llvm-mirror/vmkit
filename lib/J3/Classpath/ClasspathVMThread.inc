//===- ClasspathVMThread.cpp - GNU classpath java/lang/VMThread -----------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;

extern "C" {

// Never throws.
// Never calls Java code.
JNIEXPORT JavaObject* JNICALL Java_java_lang_VMThread_currentThread(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz
#endif
) {
  return JavaThread::get()->currentThread();
}

static void start(JavaThread* thread) {

  JavaObject* vmThread = 0;
  llvm_gcroot(vmThread, 0);

  Jnjvm* vm = thread->getJVM();

  // Ok, now that the thread is created we can set the the value of vmdata,
  // which is the JavaThread object.
  JavaField* field = vm->upcalls->vmdataVMThread;
  vmThread = thread->vmThread;
  assert(vmThread && "Didn't fix the vmThread of a j3 thread");
  JavaObject* javaThread = thread->javaThread;
  assert(javaThread && "Didn't fix the javaThread of a j3 thread");
  field->setObjectField(vmThread, (JavaObject*)(void*)thread);
  
  UserClass* vmthClass = (UserClass*)JavaObject::getClass(vmThread);
  ThreadSystem& ts = vm->threadSystem;
  
  
  // If the thread is not a daemon, it is added to the list of threads to
  // wait until exit.
  bool isDaemon = vm->upcalls->daemon->getInt8Field(javaThread);

  if (!isDaemon) {
    ts.nonDaemonLock.lock();
    ts.nonDaemonThreads++;
    ts.nonDaemonLock.unlock();
  }
  
  // Run the VMThread::run function
  vm->upcalls->runVMThread->invokeIntSpecial(vm, vmthClass, vmThread);
 
  // Remove the thread from the list.
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
JavaObject* vmThread, sint64 stackSize) {

  JavaObject* javaThread = 0;
  llvm_gcroot(vmThread, 0);
  llvm_gcroot(javaThread, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  
  // Classpath has set this field.
  javaThread = vm->upcalls->assocThread->getObjectField(vmThread);
  assert(javaThread && "VMThread with no Java equivalent");
 
  JavaThread* th = new JavaThread(javaThread, vmThread, vm);
  if (!th) vm->outOfMemoryError();
  th->start((void (*)(mvm::Thread*))start);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* vmthread) {

  llvm_gcroot(vmthread, 0);  

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmdataVMThread; 
  
  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  while (field->getObjectField(vmthread) == 0)
    mvm::Thread::yield();
  
  JavaThread* th = (JavaThread*)field->getObjectField(vmthread);
  th->interruptFlag = 1;
  JavaLock* lock = th->waitsOn;

  // If the thread is blocked on a wait. We also verify nextWaiting in case
  // the thread has been notified.
  if (lock && th->nextWaiting) {
    th->state = JavaThread::StateInterrupted;
  
    // Make sure the thread is waiting.
    uint32 locked = 0;
    while (true) {
      locked = (lock->tryAcquire() == 0);
      if (locked || (lock->getOwner() != th && lock->getOwner() != 0))
        break;
      else mvm::Thread::yield();
    }
    
    // Interrupt the thread.
    th->varcond.signal();

    // Release the lock if we acquired it.
    if (locked) lock->release(lock->getAssociatedObject());
  }

  // Here we could also raise a signal for interrupting I/O
  
  END_NATIVE_EXCEPTION
}

// Never throws.
// Never calls Java code.
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

// Never throws.
// Never calls Java code.
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_isInterrupted(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* vmthread) {
  
  llvm_gcroot(vmthread, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmdataVMThread;
  JavaThread* th = (JavaThread*)field->getObjectField(vmthread);
  return (jboolean)th->interruptFlag;
}

// Never throws.
// Never calls Java code.
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* vmthread, jint prio) {
  // Currently not implemented
  llvm_gcroot(vmthread, 0);
}

// Never throws.
// Never calls Java code.
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* vmthread, jobject exc) {
  // Currently not implemented
  llvm_gcroot(vmthread, 0);
}

// Never throws.
// Never calls Java code.
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  mvm::Thread::yield();
}

}
