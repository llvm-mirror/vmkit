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
  
  JavaObject* res = NULL;
  JavaObject* tmp = NULL;
  llvm_gcroot(res, 0);
  llvm_gcroot(src, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  UserCommonClass* cl = JavaObject::getClass(src);
  Jnjvm* vm = JavaThread::get()->getJVM();
  if (cl->isArray()) {
    UserClassArray* array = cl->asArrayClass();
    int length = JavaArray::getSize(src);
    res = array->doNew(length, vm);
    UserCommonClass* base = array->baseClass();
    if (base->isPrimitive()) {
      int size = length << base->asPrimitiveClass()->logSize;
      memcpy((void*)((uintptr_t)res + sizeof(JavaObject) + sizeof(size_t)),
             (void*)((uintptr_t)src + sizeof(JavaObject) + sizeof(size_t)),
             size);
    } else {
      for (int i = 0; i < length; i++) {
        tmp = ArrayObject::getElement((ArrayObject*)src, i);
        ArrayObject::setElement((ArrayObject*)res, tmp, i);
      }
    }
  } else {
    assert(cl->isClass() && "Not a class!");
    res = cl->asClass()->doNew(vm);
    while (cl != NULL) {
      for (uint32 i = 0; i < cl->asClass()->nbVirtualFields; ++i) {
        JavaField& field = cl->asClass()->virtualFields[i];
        if (field.isReference()) {
          tmp = field.getInstanceObjectField(src);
          JavaObject** ptr = field.getInstanceObjectFieldPtr(res);
          mvm::Collector::objectReferenceWriteBarrier((gc*)res, (gc**)ptr, (gc*)tmp);
        } else if (field.isLong()) {
          field.setInstanceLongField(res, field.getInstanceLongField(src));
        } else if (field.isDouble()) {
          field.setInstanceDoubleField(res, field.getInstanceDoubleField(src));
        } else if (field.isInt()) {
          field.setInstanceInt32Field(res, field.getInstanceInt32Field(src));
        } else if (field.isFloat()) {
          field.setInstanceFloatField(res, field.getInstanceFloatField(src));
        } else if (field.isShort() || field.isChar()) {
          field.setInstanceInt16Field(res, field.getInstanceInt16Field(src));
        } else if (field.isByte() || field.isBoolean()) {
          field.setInstanceInt8Field(res, field.getInstanceInt8Field(src));
        } else {
          UNREACHABLE();
        }
      }
      cl = cl->super;
    }
  }

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
