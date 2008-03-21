//===---------- ClasspathVMClass.h - Classpath methods --------------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JAVA_LANG_VMCLASS_H
#define _JAVA_LANG_VMCLASS_H

#include <jni.h>

extern "C" {

/*
 * Class:     java/lang/VMClass
 * Method:    isArray
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isArray(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject klass);

/*
 * Class:     java/lang/VMClass
 * Method:    forName
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClass_forName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject str, jboolean clinit, jobject loader);

/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredConstructors(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject cl, jboolean publicOnly);

/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredMethods
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Method;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredMethods(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass cl, jboolean publicOnly);

/*
 * Class:     java/lang/VMClass
 * Method:    getModifiers
 * Signature: (Ljava/lang/Class;Z)I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMClass_getModifiers(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl, jboolean ignore);

/*
 * Class:     java/lang/VMClass
 * Method:    getName
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    isPrimitive
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isPrimitive(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    isInterface
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isInterface(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    getComponentType
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getComponentType(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    getClassLoader
 * Signature: (Ljava/lang/Class;)Ljava/lang/ClassLoader;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isAssignableFrom(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl1, jclass Cl2);

/*
 * Class:     java/lang/VMClass
 * Method:    getSuperclass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getSuperclass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);



/*
 * Class:     java/lang/VMClass
 * Method:    isInstance
 * Signature: (Ljava/lang/Class;Ljava/lang/Object;)Z
 */
JNIEXPORT bool JNICALL Java_java_lang_VMClass_isInstance(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass par1, jobject par2);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredFields
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredFields(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass par1, jboolean publicOnly);










/*
 * Class:     java/lang/VMClass
 * Method:    getInterfaces
 * Signature: (Ljava/lang/Class;)[Ljava/lang/Class;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getInterfaces(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass par1);






/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaringClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getDeclaringClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl);

/*
 * Class:     java/lang/VMClass
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject throwable);

#if 0
/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredClasses
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredClasses(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1, s4 par2);













/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredAnnotations
 * Signature: (Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredAnnotations(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getEnclosingClass(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingConstructor
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Constructor;
 */
JNIEXPORT struct java_lang_reflect_Constructor* JNICALL Java_java_lang_VMClass_getEnclosingConstructor(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingMethod
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Method;
 */
JNIEXPORT struct java_lang_reflect_Method* JNICALL Java_java_lang_VMClass_getEnclosingMethod(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getClassSignature
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMClass_getClassSignature(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isAnonymousClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isAnonymousClass(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isLocalClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isLocalClass(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isMemberClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isMemberClass(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jclass clazz, struct java_lang_Class* par1);
#endif
}
#endif

