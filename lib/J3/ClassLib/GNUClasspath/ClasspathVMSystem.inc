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
JavaObject* src,
jint sstart,
JavaObject* dst,
jint dstart,
jint len) {

  JavaObject* cur = 0;
  llvm_gcroot(src, 0);
  llvm_gcroot(dst, 0);
  llvm_gcroot(cur, 0);
  assert(src->getVirtualTable());
  assert(dst->getVirtualTable());

  JavaThread* th = JavaThread::get();
  Jnjvm *vm = th->getJVM();

  if (src == NULL || dst == NULL) {
    th->pendingException = vm->CreateNullPointerException();
    return;
  }
  
  if (!(JavaObject::getClass(src)->isArray() &&
        JavaObject::getClass(dst)->isArray())) {
    th->pendingException = vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable());
    return;
  }
  
  UserClassArray* ts = (UserClassArray*)JavaObject::getClass(src);
  UserClassArray* td = (UserClassArray*)JavaObject::getClass(dst);
  UserCommonClass* dstType = td->baseClass();
  UserCommonClass* srcType = ts->baseClass();

  sint32 srcSize = JavaArray::getSize(src);
  sint32 dstSize = JavaArray::getSize(dst);

  if (len > srcSize) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len);
  } else if (len > dstSize) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len);
  } else if (len + sstart > srcSize) {
    th->pendingException = vm->CreateIndexOutOfBoundsException(len + sstart);
  } else if (len + dstart > dstSize) {
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
  
  if (!(dstType->isPrimitive())) {
    for (int i = 0; i < len; i++) {
      cur = ArrayObject::getElement((ArrayObject*)src, i + sstart);
      if (cur) {
        if (!(JavaObject::getClass(cur)->isAssignableFrom(dstType))) {
          th->pendingException = vm->CreateArrayStoreException(
              (JavaVirtualTable*)dst->getVirtualTable());
          break;
        } else {
          ArrayObject::setElement((ArrayObject*)dst, cur, i + dstart);
        }
      }
    }
  } else { 
    uint32 logSize = dstType->asPrimitiveClass()->logSize;
    void* ptrDst = (void*)((int64_t)JavaArray::getElements(dst) + (dstart << logSize));
    void* ptrSrc = (void*)((int64_t)JavaArray::getElements(src) + (sstart << logSize));
    memmove(ptrDst, ptrSrc, len << logSize);
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
  return JavaObject::hashCode(obj);
}

}
