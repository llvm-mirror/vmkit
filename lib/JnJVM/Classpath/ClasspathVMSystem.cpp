//===-- ClasspathVMSystem.cpp - GNU classpath java/lang/VMSystem ----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "llvm/Type.h"

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
  jnjvm::Jnjvm *vm = JavaThread::get()->isolate;
  JavaArray* src = (JavaArray*)_src;
  JavaArray* dst = (JavaArray*)_dst;

  verifyNull(src);
  verifyNull(dst);
  
  if (!(src->classOf->isArray && dst->classOf->isArray)) {
    vm->arrayStoreException();
  }
  
  ClassArray* ts = (ClassArray*)src->classOf;
  ClassArray* td = (ClassArray*)dst->classOf;
  AssessorDesc* srcFuncs = ts->funcs();
  AssessorDesc* dstFuncs = ts->funcs();
  CommonClass* dstType = td->baseClass();

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
  } else if (srcFuncs != dstFuncs) {
    vm->arrayStoreException();
  }
  
  jint i = sstart;
  bool doThrow = false;
  if (srcFuncs == AssessorDesc::dTab || srcFuncs == AssessorDesc::dRef) {
    while (i < sstart + len && !doThrow) {
      JavaObject* cur = ((ArrayObject*)src)->at(i);
      if (cur) {
        if (!(cur->classOf->isAssignableFrom(dstType))) {
          doThrow = true;
          len = i;
        }
      }
      ++i;
    }
  }
  
  uint32 size = srcFuncs->llvmType->getPrimitiveSizeInBits() / 8;
  if (size == 0) size = sizeof(void*);
  void* ptrDst = (void*)((int64_t)(dst->elements) + size * dstart);
  void* ptrSrc = (void*)((int64_t)(src->elements) + size * sstart);
  memmove(ptrDst, ptrSrc, size * len);

  if (doThrow)
    vm->arrayStoreException();
  

}

JNIEXPORT jint JNICALL Java_java_lang_VMSystem_identityHashCode(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                                jclass clazz,
#endif
                                                                jobject obj) {
  return (jint)obj;
}

}
