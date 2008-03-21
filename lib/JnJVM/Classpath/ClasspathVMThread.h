//===----------- ClasspathVMThread.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMTHREAD_H
#define _JAVA_LANG_VMTHREAD_H

#include <jni.h>
#include "types.h"

extern "C" {

/*
 * Class:     java/lang/VMThread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMThread_currentThread(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

/*
 * Class:     java/lang/VMThread
 * Method:    start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj, sint64 stackSize);

/*
 * Class:     java/lang/VMThread
 * Method:    interrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread);

/*
 * Class:     java/lang/VMThread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_interrupted(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

/*
 * Class:     java/lang/VMThread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_isInterrupted(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread);

/*
 * Class:     java/lang/VMThread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread, jint prio);

/*
 * Class:     java/lang/VMThread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthread, jobject exc);

/*
 * Class:     java/lang/VMThread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

#if 0
/*
 * Class:     java/lang/VMThread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_countStackFrames(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
struct java_lang_VMThread* this);








/*
 * Class:     java/lang/VMThread
 * Method:    suspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_resume(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
struct java_lang_VMThread* this);










/*
 * Class:     java/lang/VMThread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_holdsLock(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Object* par1);

#endif
}

#endif
