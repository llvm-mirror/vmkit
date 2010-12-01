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

	mvm::Thread* mut = mvm::Thread::get();
  Jnjvm *vm = JavaThread::j3Thread(mut)->getJVM();

  if (src == NULL || dst == NULL) {
    mut->setPendingException(vm->CreateNullPointerException());
    return;
  }
  
  if (!(JavaObject::getClass(src)->isArray() &&
        JavaObject::getClass(dst)->isArray())) {
    mut->setPendingException(vm->CreateArrayStoreException((JavaVirtualTable*)dst->getVirtualTable()));
    return;
  }
  
  UserClassArray* ts = (UserClassArray*)JavaObject::getClass(src);
  UserClassArray* td = (UserClassArray*)JavaObject::getClass(dst);
  UserCommonClass* dstType = td->baseClass();
  UserCommonClass* srcType = ts->baseClass();

  sint32 srcSize = JavaArray::getSize(src);
  sint32 dstSize = JavaArray::getSize(dst);

  if (len > srcSize) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(len));
  } else if (len > dstSize) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(len));
  } else if (len + sstart > srcSize) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(len + sstart));
  } else if (len + dstart > dstSize) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(len + dstart));
  } else if (dstart < 0) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(dstart));
  } else if (sstart < 0) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(sstart));
  } else if (len < 0) {
    mut->setPendingException(vm->CreateIndexOutOfBoundsException(len));
  } else if ((dstType->isPrimitive() || srcType->isPrimitive()) &&
             srcType != dstType) {
    mut->setPendingException(vm->CreateArrayStoreException((JavaVirtualTable*)dst->getVirtualTable()));
  }
  if (mut->getPendingException() != NULL) return;
  
  jint i = sstart;
  jint length = len;
  bool doThrow = false;
  if (!(dstType->isPrimitive())) {
    while (i < sstart + len && !doThrow) {
      cur = ArrayObject::getElement((ArrayObject*)src, i);
      if (cur) {
        if (!(JavaObject::getClass(cur)->isAssignableFrom(dstType))) {
          doThrow = true;
          length = i;
        }
      }
      ++i;
    }
  }
  
  uint32 logSize = dstType->isPrimitive() ? 
    dstType->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2);

  void* ptrDst = (void*)((int64_t)JavaArray::getElements(dst) + (dstart << logSize));
  void* ptrSrc = (void*)((int64_t)JavaArray::getElements(src) + (sstart << logSize));
  memmove(ptrDst, ptrSrc, length << logSize);

  if (doThrow) {
    mut->setPendingException(vm->CreateArrayStoreException((JavaVirtualTable*)dst->getVirtualTable()));
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
