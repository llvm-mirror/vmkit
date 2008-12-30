//===------ ClasspathVMObject.cpp - GNU classpath java/lang/VMObject ------===//
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
#include "JavaThread.h"
#include "Jnjvm.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMObject_clone(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jobject _src) {
  
  JavaObject* res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObject* src = (JavaObject*)_src;
  UserCommonClass* cl = src->classOf;
  Jnjvm* vm = JavaThread::get()->getJVM();
  uint64 size = 0;
  if (cl->isArray()) {
    UserClassArray* array = cl->asArrayClass();
    UserCommonClass* base = array->baseClass();
    uint32 primSize = base->isPrimitive() ? 
      base->asPrimitiveClass()->primSize : sizeof(JavaObject*);

    size = sizeof(JavaObject) + sizeof(ssize_t) + 
                            ((JavaArray*)src)->size * primSize;
  } else {
    assert(cl->isClass() && "Not a class!");
    size = cl->asClass()->getVirtualSize();
  }
  res = (JavaObject*)
    vm->gcAllocator.allocateManagedObject(size, src->getVirtualTable());
  memcpy(res, src, size);
  res->lock.initialise();

  END_NATIVE_EXCEPTION

  return (jobject)res;
} 

JNIEXPORT jobject JNICALL Java_java_lang_VMObject_getClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _obj) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObject* obj = (JavaObject*)_obj;
  Jnjvm* vm = JavaThread::get()->getJVM();
  res = (jobject)(obj->classOf->getClassDelegatee(vm)); 

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _obj) {

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObject* obj = (JavaObject*)_obj;
  obj->notifyAll();

  END_NATIVE_EXCEPTION
}


JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _object, jlong ms, jint ns) {

  BEGIN_NATIVE_EXCEPTION(0)

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

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject obj) {

  BEGIN_NATIVE_EXCEPTION(0)

  ((JavaObject*)obj)->notify();

  END_NATIVE_EXCEPTION
}

}
