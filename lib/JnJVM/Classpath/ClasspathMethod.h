//===----------- ClasspathMethod.h - Classpath methods --------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_REFLECT_METHOD_H
#define _JAVA_LANG_REFLECT_METHOD_H


extern "C" {
/*
 * Class:     java/lang/reflect/Method
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_reflect_Method_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject meth);


/*
 * Class:     java/lang/reflect/Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_reflect_Method_getReturnType(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject meth);


/*
 * Class:     java/lang/reflect/Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT jobject JNICALL Java_java_lang_reflect_Method_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject Meth);

/*
 * Class:     java/lang/reflect/Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_java_lang_reflect_Method_invokeNative(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject Meth, jobject obj, jobject Args, jclass Cl, jint meth);


/*
 * Class:     java/lang/reflect/Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT jobjectArray JNICALL Java_java_lang_reflect_Method_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject meth);



#if 0
/*
 * Class:     java/lang/reflect/Method
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_reflect_Method_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 struct java_lang_reflect_Method* this);
#endif
}
#endif

