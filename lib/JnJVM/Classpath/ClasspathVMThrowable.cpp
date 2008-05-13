//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
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
  //int** fp =  (int**)__builtin_frame_address(0);
  Jnjvm* vm = JavaThread::get()->isolate;
  int** stack = (int**)malloc(sizeof(int*) * 100);
  int real_size = JavaJIT::getBacktrace((void**)stack, 100);
  stack[real_size] = 0;
  JavaObject* vmThrowable = Classpath::newVMThrowable->doNew(vm);
  ((JavaObject**)((uint64)vmThrowable + Classpath::vmDataVMThrowable->ptrOffset))[0] = (JavaObject*)stack;
  return (jobject)vmThrowable;
}

JavaObject* consStackElement(JavaMethod* meth, int* ip) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* methodName = vm->UTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  JavaObject* className = vm->UTF8ToStr(cl->name->internalToJava(vm, 0, cl->name->size));
  JavaObject* sourceName = 0;
  
  Attribut* sourceAtt = Attribut::lookup(&cl->attributs,
                                         Attribut::sourceFileAttribut);
  
  if (sourceAtt) {
    Reader* reader = sourceAtt->toReader(JavaThread::get()->isolate, cl->bytes,
                                         sourceAtt);
    uint16 index = reader->readU2();
    sourceName = vm->UTF8ToStr(cl->ctpInfo->UTF8At(index));
  }

  bool native = isNative(meth->access);

  JavaObject* res = (*Classpath::newStackTraceElement)(vm);
  Classpath::initStackTraceElement->invokeIntSpecial(vm, res, sourceName,
                                                     (uint32)ip, className,
                                                     methodName, native);
  return res;
}

ArrayObject* recGetStackTrace(int** stack, uint32 first, uint32 rec) {
  Jnjvm* vm = JavaThread::get()->isolate;
  if (stack[first] != 0) {
#ifdef MULTIPLE_GC
    int *begIp = (int*)mvm::Thread::get()->GC->begOf(stack[first]);
#else
    int *begIp = (int*)Collector::begOf(stack[first]);
#endif
    JavaMethod* meth = JavaJIT::IPToJavaMethod(begIp);
    if (meth) {
      ArrayObject* res = recGetStackTrace(stack, first + 1, rec + 1);
      res->setAt(rec, consStackElement(meth, stack[first]));
      return res;
    } else {
      return recGetStackTrace(stack, first + 1, rec);
    }
  } else {
    return ArrayObject::acons(rec, JavaArray::ofObject, vm);
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthrow, jobject throwable) {
  int** stack = (int**)(ArrayUInt32*)(*Classpath::vmDataVMThrowable)((JavaObject*)vmthrow).PointerVal;
  CommonClass* cl = ((JavaObject*)throwable)->classOf;
  uint32 first = 0;
  sint32 i = 0;
  
  while (stack[i] != 0) {
#ifdef MULTIPLE_GC
    int *begIp = (int*)mvm::Thread::get()->GC->begOf((void*)stack[i++]);
#else
    int *begIp = (int*)Collector::begOf((void*)stack[i++]);
#endif
    JavaMethod* meth = JavaJIT::IPToJavaMethod(begIp);
    if (meth && meth->classDef == cl) {
      first = i;
      break;
    }
  }
  free(stack);
  return (jobject)recGetStackTrace((int**)(uint32**)stack, first, 0);
}

}
