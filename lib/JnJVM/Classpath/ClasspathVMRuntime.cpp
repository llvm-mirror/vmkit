//===------ ClasspathVMRuntime.cpp - GNU classpath java/lang/VMRuntime ----===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <dlfcn.h>
#include <string.h>

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


using namespace jnjvm;

extern "C" {


JNIEXPORT jobject JNICALL Java_java_lang_VMRuntime_mapLibraryName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _strLib) {

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
  
  return (jobject)(vm->UTF8ToStr((const UTF8*)array));
  
}

typedef int (*onLoad_t)(const void**, void*);

JNIEXPORT jint JNICALL Java_java_lang_VMRuntime_nativeLoad(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str,
jobject _loader) {
  JavaString* str = (JavaString*)_str;
  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* loader = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)_loader, vm);

  char* buf = str->strToAsciiz();
  
  void* res = dlopen(buf, RTLD_LAZY | RTLD_LOCAL);
  if (res != 0) {
    loader->nativeLibs.push_back(res);
    onLoad_t onLoad = (onLoad_t)(intptr_t)dlsym(res, "JNI_OnLoad");
    if (onLoad) onLoad(&vm->javavmEnv, 0);
    return 1;
  } else {
    return 0;
  }
}


JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
#ifdef MULTIPLE_GC
    mvm::Thread::get()->GC->collect();
#else
  Collector::collect();
#endif
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
#ifdef MULTIPLE_GC
  return (jlong)JavaThread::get()->GC->getFreeMemory();
#else
  return (jlong)Collector::getFreeMemory();
#endif
}

JNIEXPORT jlong Java_java_lang_VMRuntime_totalMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
#ifdef MULTIPLE_GC
  return (jlong)mvm::Thread::get()->GC->getTotalMemory();
#else
  return (jlong)Collector::getTotalMemory();
#endif
}

JNIEXPORT jlong Java_java_lang_VMRuntime_maxMemory(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
#ifdef MULTIPLE_GC
  return (jlong)mvm::Thread::get()->GC->getMaxMemory();
#else
  return (jlong)Collector::getMaxMemory();
#endif
}

JNIEXPORT jint Java_java_lang_VMRuntime_availableProcessors(){
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
  return 1;
}
}

