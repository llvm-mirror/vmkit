//===------ ClasspathVMObject.cpp - GNU classpath java/lang/VMObject ------===//
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
#include "JavaTypes.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMObject_clone(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
                                                        jobject _src) {
  
  JavaObject* src = (JavaObject*)_src;
  uint64 size = src->objectSize() + 4; // + VT
  JavaObject* res = (JavaObject*)
    JavaThread::get()->isolate->allocateObject(size, src->getVirtualTable());
  memcpy(res, src, size);
  return (jobject)res;
} 

JNIEXPORT jobject JNICALL Java_java_lang_VMObject_getClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
                                                           jobject _obj) {
  JavaObject* obj = (JavaObject*)_obj;
  return (jobject)(obj->classOf->getClassDelegatee()); 
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif

                                                         jobject _obj) {
  JavaObject* obj = (JavaObject*)_obj;
  obj->notifyAll();
}


JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _object, jlong ms, jint ns) {
  uint32 sec = (uint32) (ms / 1000);
  uint32 usec = (ns / 1000) + 1000 * (ms % 1000);
  JavaObject* obj = (JavaObject*)_object;
  if (sec || usec) {
    struct timeval t;
    t.tv_sec = sec;
    t.tv_usec = usec;
    obj->timedWait(t);
  } else {
    obj->wait();
  }
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject obj) {
  ((JavaObject*)obj)->notify();
}

}
