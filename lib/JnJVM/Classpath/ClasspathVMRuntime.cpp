//===------ ClasspathVMRuntime.cpp - GNU classpath java/lang/VMRuntime ----===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "MvmGC.h"

#include "types.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "LockedMap.h"
#include "NativeUtil.h"

#include <cstring>

using namespace jnjvm;

extern "C" {


JNIEXPORT jobject JNICALL Java_java_lang_VMRuntime_mapLibraryName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _strLib) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaString* strLib = (JavaString*)_strLib;
  Jnjvm* vm = JavaThread::get()->getJVM();

  const UTF8* utf8Lib = strLib->value;
  uint32 stLib = strLib->offset;
  sint32 lgLib = strLib->count;
  sint32 lgPre = vm->bootstrapLoader->prelib->size;
  sint32 lgPost = vm->bootstrapLoader->postlib->size;
  
  uint32 size = (uint32)(lgPre + lgLib + lgPost);
  ArrayUInt16* array = (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(size, vm);
  uint16* elements = array->elements;

  memmove(elements, vm->bootstrapLoader->prelib->elements,
          lgPre * sizeof(uint16));
  memmove(&(elements[lgPre]), &(utf8Lib->elements[stLib]), 
          lgLib * sizeof(uint16));
  memmove(&(elements[lgPre + lgLib]), vm->bootstrapLoader->postlib->elements,
           lgPost * sizeof(uint16));
  
  res = (jobject)(vm->UTF8ToStr((const UTF8*)array));

  END_NATIVE_EXCEPTION

  return res;
  
}

typedef int (*onLoad_t)(const void**, void*);

JNIEXPORT jint JNICALL Java_java_lang_VMRuntime_nativeLoad(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str,
jobject _loader) {
  
  jint result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaString* str = (JavaString*)_str;
  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* loader = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)_loader, vm);

  char* buf = str->strToAsciiz();
  
  void* res = loader->loadLib(buf);
  
  if (res != 0) {
    onLoad_t onLoad = (onLoad_t)loader->loadInLib("JNI_OnLoad", res);
    if (onLoad) onLoad(&vm->javavmEnv, 0);
    result = 1;
  } else {
    result = 0;
  }

  END_NATIVE_EXCEPTION

  return result;
}


JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  BEGIN_NATIVE_EXCEPTION(0)
  
  Collector::collect();

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return;
}

JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return;
}

JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
uint8 value
) {
  return;
}

JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jint par1) {
#if defined(ISOLATE) || defined(ISOLATE_SHARING)
  // TODO: do a longjmp
  exit(par1);
#else
  exit(par1);
#endif
}

JNIEXPORT jlong Java_java_lang_VMRuntime_freeMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return (jlong)Collector::getFreeMemory();
}

JNIEXPORT jlong Java_java_lang_VMRuntime_totalMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return (jlong)Collector::getTotalMemory();
}

JNIEXPORT jlong Java_java_lang_VMRuntime_maxMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return (jlong)Collector::getMaxMemory();
}

JNIEXPORT jint Java_java_lang_VMRuntime_availableProcessors(){
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
  return 1;
}
}

