//===-- ClasspathVMSystem.cpp - GNU classpath java/lang/VMSystem ----------===//
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

JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass _cl,
#endif
jobject _src,
jint sstart,
jobject _dst,
jint dstart,
jint len) {
  
  BEGIN_NATIVE_EXCEPTION(0)

  jnjvm::Jnjvm *vm = JavaThread::get()->getJVM();
  JavaArray* src = (JavaArray*)_src;
  JavaArray* dst = (JavaArray*)_dst;

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
      JavaObject* cur = ((ArrayObject*)src)->elements[i];
      if (cur) {
        if (!(cur->getClass()->isAssignableFrom(dstType))) {
          doThrow = true;
          len = i;
        }
      }
      ++i;
    }
  }
  
  uint32 size = dstType->isPrimitive() ? 
                  dstType->asPrimitiveClass()->primSize : sizeof(JavaObject*);
  
  assert(size && "Size zero in a arraycopy");
  void* ptrDst = (void*)((int64_t)(dst->elements) + size * dstart);
  void* ptrSrc = (void*)((int64_t)(src->elements) + size * sstart);
  memmove(ptrDst, ptrSrc, size * len);

  if (doThrow)
    vm->arrayStoreException();
  
  
  END_NATIVE_EXCEPTION

}

JNIEXPORT jint JNICALL Java_java_lang_VMSystem_identityHashCode(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject obj) {
  return (jint)(intptr_t)obj;
}

}
