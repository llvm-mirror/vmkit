//===------ ClasspathVMClassLoader.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMCLASSLOADER_H
#define _JAVA_LANG_VMCLASSLOADER_H

#include <jni.h>

extern "C" {

/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (C)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jchar par1);

/*
 * Class:     java/lang/VMClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_findLoadedClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject loader, jobject name);

/*
 * Class:     java/lang/VMClassLoader
 * Method:    loadClass
 * Signature: (Ljava/lang/String;Z)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject str, jboolean doResolve);


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_defineClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject loader, jobject str, jobject bytes, jint off, jint len, jobject pd);


/*
 * Class:     java/lang/VMClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);



#if 0
/*
 * Class:     java/lang/VMClassLoader
 * Method:    nativeGetResources
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT struct java_util_Vector* JNICALL Java_java_lang_VMClassLoader_nativeGetResources(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_String* par1);




#endif

}

#endif

