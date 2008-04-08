//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "llvm/Type.h"

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
  
  AssessorDesc* ass = AssessorDesc::byteIdToPrimitive(byteId);
  Jnjvm* vm = JavaThread::get()->isolate;
  if (!ass)
    vm->unknownError("unknown byte primitive %c", byteId);
  
  return (jobject)ass->classType->getClassDelegatee();
  
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
  CommonClass* cl = vm->lookupClass(utf8, (JavaObject*)loader);

  if (cl) return (jclass)(cl->getClassDelegatee());
  else return 0;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
                                                                jobject _str, 
                                                                jboolean doResolve) {
  JavaString* str = (JavaString*)_str;
  Jnjvm* vm = JavaThread::get()->isolate;

  CommonClass* cl = vm->lookupClassFromJavaString(str, 0, doResolve, false, false);

  if (cl != 0) {
    return (jclass)cl->getClassDelegatee();
  } else {
    return 0;
  }
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
  JavaString* str = (JavaString*)_str;
  const UTF8* name = str->value->javaToInternal(vm, str->offset, str->count);
  Class* cl = vm->constructClass(name, (JavaObject*)loader);

  if (cl->status == hashed) {
    cl->aquire();
    if (cl->status == hashed) {
      cl->bytes = (ArrayUInt8*)bytes;
      cl->status = loaded;
#ifndef MULTIPLE_VM
      cl->delegatee = (JavaObject*)pd;
#else
      vm->delegatees->hash(cl, (JavaObject*)pd);
#endif
    }
    cl->release();
  }
  return (jclass)(cl->getClassDelegatee());
}

JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
                                                                 jclass Cl) {
  verifyNull(Cl);
  NativeUtil::resolvedImplClass(Cl, false);
}

}
