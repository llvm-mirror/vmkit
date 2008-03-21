//===------------ ClasspathVMField.h - Classpath methods ------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_REFLECT_FIELD_H
#define _JAVA_LANG_REFLECT_FIELD_H

#include <jni.h>

extern "C" {

/*
 * Class:     java/lang/reflect/Field
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject field);

/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj);

/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj);

/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj);


/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jobject val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jbyte val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val);


/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val);

#if 0
/*
 * Class:     java/lang/reflect/Field
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_reflect_Field_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
struct java_lang_reflect_Field* this);
#endif
}
#endif


