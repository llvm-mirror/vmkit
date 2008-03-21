//===-------- ClasspathConstructor.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef _JAVA_LANG_REFLECT_CONSTRUCTOR_H
#define _JAVA_LANG_REFLECT_CONSTRUCTOR_H

#include <jni.h>

extern "C" {

/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
jobject cons
);

/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_reflect_Constructor_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
jobject cons);


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    constructNative
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_constructNative(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject cons, jobject args, jclass Clazz, jint meth);



/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT jobjectArray JNICALL Java_java_lang_reflect_Constructor_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
jobject cons);



#if 0
/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_reflect_Constructor_getSignature(JNIEnv *env, struct java_lang_reflect_Constructor* this);

#endif

}
#endif
