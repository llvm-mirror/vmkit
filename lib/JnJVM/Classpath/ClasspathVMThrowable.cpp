//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstring>

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
  Jnjvm* vm = JavaThread::get()->getJVM();
  int** stack = 
    (int**)vm->gcAllocator.allocateTemporaryMemory(sizeof(int*) * 100);
  int real_size = mvm::MvmModule::getBacktrace((void**)stack, 100);
  stack[real_size] = 0;
  JavaObject* vmThrowable = vm->upcalls->newVMThrowable->doNew(vm);
  uint64 ptr = (uint64)vmThrowable + vm->upcalls->vmDataVMThrowable->ptrOffset;
  ((JavaObject**)ptr)[0] = (JavaObject*)stack;
  return (jobject)vmThrowable;
}


JavaObject* consStackElement(JavaMethod* meth, int* ip) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaObject* methodName = vm->internalUTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  const UTF8* internal = cl->name->internalToJava(vm, 0, cl->name->size);
  JavaObject* className = vm->UTF8ToStr(internal);
  JavaObject* sourceName = 0;
  
  Attribut* sourceAtt = cl->lookupAttribut(Attribut::sourceFileAttribut);
  
  if (sourceAtt) {
    Reader reader(sourceAtt, cl->getBytes());
    uint16 index = reader.readU2();
    sourceName = vm->internalUTF8ToStr(cl->getConstantPool()->UTF8At(index));
  }

  bool native = isNative(meth->access);

  UserClass* newS = vm->upcalls->newStackTraceElement;
  JavaObject* res = newS->doNew(vm);
  vm->upcalls->initStackTraceElement->invokeIntSpecial(vm, newS, res,
                                                       sourceName,
                                                       0, // source line
                                                       className,
                                                       methodName, native);
  return res;
}

ArrayObject* recGetStackTrace(int** stack, uint32 first, uint32 rec) {
  Jnjvm* vm = JavaThread::get()->getJVM();
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
    return (ArrayObject*)vm->upcalls->stackTraceArray->doNew(rec, vm);
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthrow, jobject throwable) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmDataVMThrowable;
  int** stack = (int**)field->getObjectField((JavaObject*)vmthrow);
  uint32 first = 0;
  sint32 i = 0;
  
  while (stack[i] != 0) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(stack[i++]);
#ifdef ISOLATE_SHARING
    if (meth) {
#else
    if (meth && !meth->classDef->subclassOf(vm->upcalls->newThrowable)) {
#endif
      first = i - 1;
      break;
    }
  }
  jobject res = (jobject)recGetStackTrace((int**)(uint32**)stack, first, 0);
  vm->gcAllocator.freeTemporaryMemory(stack);
  return res;
}

}

