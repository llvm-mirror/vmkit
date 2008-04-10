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

#include <execinfo.h>

using namespace jnjvm;

extern "C" int backtrace_fp(int** ips, int size, int**);
extern "C" JavaMethod* ip_to_meth(int* ip);

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_fillInStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
                                                                      jobject throwable) {
  //int** fp =  (int**)__builtin_frame_address(0);
  Jnjvm* vm = JavaThread::get()->isolate;
  int** stack = (int**)alloca(sizeof(int*) * 100);
  int real_size = backtrace((void**)stack, 100);
  ArrayUInt32* obj = ArrayUInt32::acons(real_size, JavaArray::ofInt, vm);
  memcpy(obj->elements, stack, real_size * sizeof(int));
  JavaObject* vmThrowable = (*Classpath::newVMThrowable)(vm);
  Classpath::initVMThrowable->invokeIntSpecial(vmThrowable);
  (*Classpath::vmDataVMThrowable)(vmThrowable, obj);
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
  Classpath::initStackTraceElement->invokeIntSpecial(res, sourceName, (uint32)ip, className, methodName, native);
  return res;
}

ArrayObject* recGetStackTrace(int** stack, uint32 size, uint32 first, uint32 rec) {
  Jnjvm* vm = JavaThread::get()->isolate;
  if (size != first) {
    int *begIp = (int*)Collector::begOf(stack[first]);
    JavaMethod* meth = ip_to_meth(begIp);
    if (meth) {
      ArrayObject* res = recGetStackTrace(stack, size, first + 1, rec + 1);
      res->setAt(rec, consStackElement(meth, stack[first]));
      return res;
    } else {
      return recGetStackTrace(stack, size, first + 1, rec);
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
  ArrayUInt32* array = (ArrayUInt32*)(*Classpath::vmDataVMThrowable)((JavaObject*)vmthrow).PointerVal;
  uint32* stack = array->elements;
  CommonClass* cl = ((JavaObject*)throwable)->classOf;
  uint32 first = 0;
  sint32 i = 0;
  
  while (i < array->size) {
    int *begIp = (int*)Collector::begOf((void*)stack[i++]);
    JavaMethod* meth = ip_to_meth(begIp);
    if (meth && meth->classDef == cl) {
      first = i;
      break;
    }
  }
  return (jobject)recGetStackTrace((int**)(uint32**)stack, array->size, first, 0);
}

}
