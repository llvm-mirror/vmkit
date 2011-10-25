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
    th->throwException(vm->CreateNullPointerException());
    UNREACHABLE();
  }
  
  if (!(JavaObject::getClass(src)->isArray() &&
        JavaObject::getClass(dst)->isArray())) {
    th->throwException(vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable()));
    UNREACHABLE();
  }
  
  UserClassArray* ts = (UserClassArray*)JavaObject::getClass(src);
  UserClassArray* td = (UserClassArray*)JavaObject::getClass(dst);
  UserCommonClass* dstType = td->baseClass();
  UserCommonClass* srcType = ts->baseClass();

  sint32 srcSize = JavaArray::getSize(src);
  sint32 dstSize = JavaArray::getSize(dst);

  if (len > srcSize) {
    th->throwException(vm->CreateIndexOutOfBoundsException(len));
  } else if (len > dstSize) {
    th->throwException(vm->CreateIndexOutOfBoundsException(len));
  } else if (len + sstart > srcSize) {
    th->throwException(vm->CreateIndexOutOfBoundsException(len + sstart));
  } else if (len + dstart > dstSize) {
    th->throwException(vm->CreateIndexOutOfBoundsException(len + dstart));
  } else if (dstart < 0) {
    th->throwException(vm->CreateIndexOutOfBoundsException(dstart));
  } else if (sstart < 0) {
    th->throwException(vm->CreateIndexOutOfBoundsException(sstart));
  } else if (len < 0) {
    th->throwException(vm->CreateIndexOutOfBoundsException(len));
  } else if ((dstType->isPrimitive() || srcType->isPrimitive()) &&
             srcType != dstType) {
    th->throwException(vm->CreateArrayStoreException(
      (JavaVirtualTable*)dst->getVirtualTable()));
  }
  
  if (!(dstType->isPrimitive())) {
    for (int i = 0; i < len; i++) {
      cur = ArrayObject::getElement((ArrayObject*)src, i + sstart);
      if (cur) {
        if (!(JavaObject::getClass(cur)->isAssignableFrom(dstType))) {
          th->throwException(vm->CreateArrayStoreException(
              (JavaVirtualTable*)dst->getVirtualTable()));
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
