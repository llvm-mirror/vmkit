//===-------- ClasspathVMThrowable.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMTHROWABLE_H
#define _JAVA_LANG_VMTHROWABLE_H

extern "C" {
/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_fillInStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject throwable);

/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj, jobject par1);

}
#endif

