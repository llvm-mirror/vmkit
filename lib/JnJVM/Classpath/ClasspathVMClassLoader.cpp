//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "types.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "LockedMap.h"
#include "NativeUtil.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jchar byteId) {
  
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClassPrimitive* prim = 
    UserClassPrimitive::byteIdToPrimitive(byteId, vm->upcalls);
  if (!prim)
    vm->unknownError("unknown byte primitive %c", byteId);
  
  return (jobject)prim->getClassDelegatee(vm);
  
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_findLoadedClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject loader, 
jobject _name) {

  Jnjvm* vm = JavaThread::get()->isolate;
  JavaString* name = (JavaString*)_name;
  const UTF8* utf8 = name->strToUTF8(vm);
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  UserCommonClass* cl = JCL->lookupClass(utf8);

  if (cl) return (jclass)(cl->getClassDelegatee(vm));
  
  return 0;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str, 
jboolean doResolve) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaString* str = (JavaString*)_str;

  JnjvmClassLoader* JCL = vm->bootstrapLoader;
  UserCommonClass* cl = JCL->lookupClassFromJavaString(str, doResolve, false);

  if (cl != 0)
    return (jclass)cl->getClassDelegatee(vm);
  
  return 0;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_defineClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jobject loader, 
jobject _str, 
jobject bytes, 
jint off, 
jint len, 
jobject pd) {
  Jnjvm* vm = JavaThread::get()->isolate;
  
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  
  JavaString* str = (JavaString*)_str;
  const UTF8* name = str->value->javaToInternal(vm->hashUTF8, str->offset,
                                                str->count);
  UserClass* cl = JCL->constructClass(name, (ArrayUInt8*)bytes);

  return (jclass)(cl->getClassDelegatee(vm, (JavaObject*)pd));
}

JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {
  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->isolate;
  NativeUtil::resolvedImplClass(vm, Cl, false);
}

}
