//===-------- ClasspathVMRuntime.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMRUNTIME_H
#define _JAVA_LANG_VMRUNTIME_H

#include <jni.h>

extern "C" {

/*
 * Class:     java/lang/VMRuntime
 * Method:    mapLibraryName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMRuntime_mapLibraryName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject strLib);

/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMRuntime_nativeLoad(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str, jobject _loader);

/*
 * Class:     java/lang/VMRuntime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizationForExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

/*
 * Class:     java/lang/VMRuntime
 * Method:    exit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jint par1);

/*
 * Class:     java/lang/VMRuntime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_freeMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz
#endif
);


/*
 * Class:     java/lang/VMRuntime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_totalMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz
#endif
);


/*
 * Class:     java/lang/VMRuntime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_maxMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz
#endif
);

#if 0
/*
 * Class:     java/lang/VMRuntime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMRuntime_availableProcessors(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz);










/*
 * Class:     java/lang/VMRuntime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceInstructions(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceMethodCalls(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, s4 par1);







#endif

}

#endif
