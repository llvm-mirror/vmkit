//===-- ClasspathVMSystem.cpp - GNU classpath java/lang/VMSystem ----------===//
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

JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass _cl,
#endif
JavaArray* src,
jint sstart,
JavaArray* dst,
jint dstart,
jint len) {

  JavaObject* cur = 0;
  llvm_gcroot(src, 0);
  llvm_gcroot(dst, 0);
  llvm_gcroot(cur, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  j3::Jnjvm *vm = JavaThread::get()->getJVM();

  verifyNull(src);
  verifyNull(dst);
  
  if (!(src->getClass()->isArray() && dst->getClass()->isArray())) {
    vm->arrayStoreException();
  }
  
  UserClassArray* ts = (UserClassArray*)src->getClass();
  UserClassArray* td = (UserClassArray*)dst->getClass();
  UserCommonClass* dstType = td->baseClass();
  UserCommonClass* srcType = ts->baseClass();

  if (len > src->size) {
    vm->indexOutOfBounds(src, len);
  } else if (len > dst->size) {
    vm->indexOutOfBounds(dst, len);
  } else if (len + sstart > src->size) {
    vm->indexOutOfBounds(src, len + sstart);
  } else if (len + dstart > dst->size) {
    vm->indexOutOfBounds(dst, len + dstart);
  } else if (dstart < 0) {
    vm->indexOutOfBounds(dst, dstart);
  } else if (sstart < 0) {
    vm->indexOutOfBounds(src, sstart);
  } else if (len < 0) {
    vm->indexOutOfBounds(src, len);
  } else if ((dstType->isPrimitive() || srcType->isPrimitive()) &&
             srcType != dstType) {
    vm->arrayStoreException();
  }
  
  jint i = sstart;
  bool doThrow = false;
  if (!(dstType->isPrimitive())) {
    while (i < sstart + len && !doThrow) {
      cur = ((ArrayObject*)src)->elements[i];
      if (cur) {
        if (!(cur->getClass()->isAssignableFrom(dstType))) {
          doThrow = true;
          len = i;
        }
      }
      ++i;
    }
  }
  
  uint32 logSize = dstType->isPrimitive() ? 
    dstType->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2);

  void* ptrDst = (void*)((int64_t)(dst->elements) + (dstart << logSize));
  void* ptrSrc = (void*)((int64_t)(src->elements) + (sstart << logSize));
  memmove(ptrDst, ptrSrc, len << logSize);

  if (doThrow)
    vm->arrayStoreException();
  
  
  END_NATIVE_EXCEPTION

}

JNIEXPORT jint JNICALL Java_java_lang_VMSystem_identityHashCode(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);
  return (jint)(intptr_t)obj;
}

}
