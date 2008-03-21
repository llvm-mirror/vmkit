//===------- ClasspathVMStackWalker.h - Classpath methods -----------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GNU_CLASSPATH_VMSTACKWALKER_H
#define _GNU_CLASSPATH_VMSTACKWALKER_H

extern "C" {

/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
);

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

#if 0
/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_gnu_classpath_VMStackWalker_getCallingClass(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz);


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getCallingClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz);
#endif

}

#endif
