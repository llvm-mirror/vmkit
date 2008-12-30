//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "Jnjvm.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jchar byteId) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassPrimitive* prim = 
    UserClassPrimitive::byteIdToPrimitive(byteId, vm->upcalls);
  if (!prim)
    vm->unknownError("unknown byte primitive %c", byteId);
  
  res = (jobject)prim->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
  
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_findLoadedClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject loader, 
jobject _name) {
  
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* name = (JavaString*)_name;
  const UTF8* utf8 = name->strToUTF8(vm);
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  UserCommonClass* cl = JCL->lookupClass(utf8);

  if (cl) res = (jclass)(cl->getClassDelegatee(vm));

  END_NATIVE_EXCEPTION

  return res;
  
  return 0;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str, 
jboolean doResolve) {

  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* str = (JavaString*)_str;

  JnjvmClassLoader* JCL = vm->bootstrapLoader;
  UserCommonClass* cl = JCL->lookupClassFromJavaString(str, vm, doResolve,
                                                       false);

  if (cl != 0)
    res = (jclass)cl->getClassDelegatee(vm);
  
  END_NATIVE_EXCEPTION

  return res;
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
  
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  
  JavaString* str = (JavaString*)_str;
  const UTF8* name = str->value->javaToInternal(vm, str->offset, str->count);
  UserClass* cl = JCL->constructClass(name, (ArrayUInt8*)bytes);

  res = (jclass)(cl->getClassDelegatee(vm, (JavaObject*)pd));

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass::resolvedImplClass(vm, (JavaObject*)Cl, false);

  END_NATIVE_EXCEPTION
}

}
