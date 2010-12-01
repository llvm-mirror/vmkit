//===------ ClasspathVMObject.cpp - GNU classpath java/lang/VMObject ------===//
//
//                            The VMKit project
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

using namespace j3;

extern "C" {

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMObject_clone(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* src) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(src, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  UserCommonClass* cl = JavaObject::getClass(src);
  size_t size = 0;
  if (cl->isArray()) {
    UserClassArray* array = cl->asArrayClass();
    UserCommonClass* base = array->baseClass();
    uint32 logSize = base->isPrimitive() ? 
      base->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2);

    size = sizeof(JavaObject) + sizeof(ssize_t) + 
                  (JavaArray::getSize(src) << logSize);
  } else {
    assert(cl->isClass() && "Not a class!");
    size = cl->asClass()->getVirtualSize();
  }
  res = (JavaObject*)mvm::gc::operator new(size, src->getVirtualTable());
  memcpy((void*)((uintptr_t)res + 2 * sizeof(void*)),
         (void*)((uintptr_t)src + 2 * sizeof(void*)),
         size - 2 * sizeof(void*));

  END_NATIVE_EXCEPTION

  return res;
} 

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMObject_getClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  res = JavaObject::getClass(obj)->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObject::notifyAll(obj);

  END_NATIVE_EXCEPTION
}


JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj, jlong ms, jint ns) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  uint32 sec = (uint32) (ms / 1000);
  uint32 usec = (ns / 1000) + 1000 * (ms % 1000);
  if (ns && !usec) usec = 1;
  if (sec || usec) {
    struct timeval t;
    t.tv_sec = sec;
    t.tv_usec = usec;
    JavaObject::timedWait(obj, t);
  } else {
    JavaObject::wait(obj);
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaObject::notify(obj);

  END_NATIVE_EXCEPTION
}

}
