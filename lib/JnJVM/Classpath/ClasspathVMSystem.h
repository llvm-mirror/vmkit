//===----------- ClasspathVMSystem.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMSYSTEM_H
#define _JAVA_LANG_VMSYSTEM_H

#include <jni.h>


extern "C" {
/*
 * Class:     java/lang/VMSystem
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject par1, jint par2, jobject par3, jint par4, jint par5);


/*
 * Class:     java/lang/VMSystem
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMSystem_identityHashCode(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject obj);


#if 0
/*
 * Class:     java/lang/VMSystem
 * Method:    setIn
 * Signature: (Ljava/io/InputStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setIn(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_io_InputStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    setOut
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setOut(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_io_PrintStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    setErr
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setErr(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_io_PrintStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    currentTimeMillis
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMSystem_currentTimeMillis(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz);


/*
 * Class:     java/lang/VMSystem
 * Method:    getenv
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMSystem_getenv(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_String* par1);

#endif
}
#endif
