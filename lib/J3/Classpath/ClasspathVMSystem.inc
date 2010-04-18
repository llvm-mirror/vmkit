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

  JavaThread* th = JavaThread::get();
  Jnjvm *vm = th->getJVM();

  if (src == NULL || dst == NULL) {
    th->pendingException = vm->CreateNullPointerException();
    return;
  }
  
  if (!(src->getClass()->isArray() && dst->getClass()->isArray())) {
    th->pendingException = vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable());
    return;
  }
  
  UserClassArray* ts = (UserClassArray*)src->getClass();
  UserClassArray* td = (UserClassArray*)dst->getClass();
  UserCommonClass* dstType = td->baseClass();
  UserCommonClass* srcType = ts->baseClass();

  if (len > src->size) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len);
  } else if (len > dst->size) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len);
  } else if (len + sstart > src->size) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len + sstart);
  } else if (len + dstart > dst->size) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len + dstart);
  } else if (dstart < 0) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(dstart);
  } else if (sstart < 0) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(sstart);
  } else if (len < 0) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len);
  } else if ((dstType->isPrimitive() || srcType->isPrimitive()) &&
             srcType != dstType) {
    th->pendingException = vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable());
  }
  if (th->pendingException != NULL) return;
  
  jint i = sstart;
  jint length = len;
  bool doThrow = false;
  if (!(dstType->isPrimitive())) {
    while (i < sstart + len && !doThrow) {
      cur = ((ArrayObject*)src)->elements[i];
      if (cur) {
        if (!(cur->getClass()->isAssignableFrom(dstType))) {
          doThrow = true;
          length = i;
        }
      }
      ++i;
    }
  }
  
  uint32 logSize = dstType->isPrimitive() ? 
    dstType->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2);

  void* ptrDst = (void*)((int64_t)(dst->elements) + (dstart << logSize));
  void* ptrSrc = (void*)((int64_t)(src->elements) + (sstart << logSize));
  memmove(ptrDst, ptrSrc, length << logSize);

  if (doThrow) {
    th->pendingException = vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable());
  }
}

JNIEXPORT jint JNICALL Java_java_lang_VMSystem_identityHashCode(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);
  if (obj == NULL) return 0;
  return obj->hashCode();
}

}
