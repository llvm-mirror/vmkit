//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "types.h"

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"
#include "Reader.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_fillInStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
                                                                      jobject throwable) {
  Jnjvm* vm = JavaThread::get()->isolate;
  int** stack = (int**)malloc(sizeof(int*) * 100);
  int real_size = mvm::jit::getBacktrace((void**)stack, 100);
  stack[real_size] = 0;
  JavaObject* vmThrowable = vm->upcalls->newVMThrowable->doNew(vm);
  ((JavaObject**)((uint64)vmThrowable + vm->upcalls->vmDataVMThrowable->ptrOffset))[0] = (JavaObject*)stack;
  return (jobject)vmThrowable;
}


#ifndef MULTIPLE_VM
JavaObject* consStackElement(JavaMethod* meth, int* ip) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* methodName = vm->UTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  JavaObject* className = vm->UTF8ToStr(cl->name->internalToJava(cl->classLoader->hashUTF8, 0, cl->name->size));
  JavaObject* sourceName = 0;
  
  Attribut* sourceAtt = cl->lookupAttribut(Attribut::sourceFileAttribut);
  
  if (sourceAtt) {
    Reader reader(sourceAtt, cl->getBytes());
    uint16 index = reader.readU2();
    sourceName = vm->UTF8ToStr(cl->getConstantPool()->UTF8At(index));
  }

  bool native = isNative(meth->access);

  UserClass* newS = vm->upcalls->newStackTraceElement;
  JavaObject* res = newS->doNew(vm);
  vm->upcalls->initStackTraceElement->invokeIntSpecial(vm, newS, res,
                                                       sourceName,
                                                       (uint32)ip, className,
                                                       methodName, native);
  return res;
}

ArrayObject* recGetStackTrace(int** stack, uint32 first, uint32 rec) {
  Jnjvm* vm = JavaThread::get()->isolate;
  if (stack[first] != 0) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(stack[first]);
    if (meth) {
      ArrayObject* res = recGetStackTrace(stack, first + 1, rec + 1);
      res->elements[rec] = consStackElement(meth, stack[first]);
      return res;
    } else {
      return recGetStackTrace(stack, first + 1, rec);
    }
  } else {
    return ArrayObject::acons(rec, vm->upcalls->stackTraceArray, &(vm->allocator));
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthrow, jobject throwable) {
  Jnjvm* vm = JavaThread::get()->isolate;
  int** stack = (int**)vm->upcalls->vmDataVMThrowable->getObjectField((JavaObject*)vmthrow);
  uint32 first = 0;
  sint32 i = 0;
  
  while (stack[i] != 0) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(stack[i++]);
    if (meth && !meth->classDef->subclassOf(vm->upcalls->newThrowable)) {
      first = i - 1;
      break;
    }
  }
  jobject res = (jobject)recGetStackTrace((int**)(uint32**)stack, first, 0);
  free(stack);
  return res;
}

#endif
}

