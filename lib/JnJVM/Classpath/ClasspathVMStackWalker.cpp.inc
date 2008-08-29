//===- ClasspathVMStackWalker.cpp -----------------------------------------===//
//===------------ GNU classpath gnu/classpath/VMStackWalker ---------------===//
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

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

using namespace jnjvm;

extern "C" {

#ifndef MULTIPLE_VM
ArrayObject* recGetClassContext(Jnjvm* vm, int** stack, uint32 size, uint32 first, uint32 rec) {
  if (size != first) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(stack[first]);
    if (meth) {
      ArrayObject* res = recGetClassContext(vm, stack, size, first + 1, rec + 1); 
      res->elements[rec] = meth->classDef->getClassDelegatee(vm);
      return res;
    } else {
      return recGetClassContext(vm, stack, size, first + 1, rec);
    }   
  } else {
    return ArrayObject::acons(rec, vm->upcalls->classArrayClass, &(vm->allocator));
  }
}

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  Jnjvm* vm = JavaThread::get()->isolate;
  int* ips[100];
  int real_size = mvm::jit::getBacktrace((void**)(void*)ips, 100);
  int i = 0;
  int first = 0;
  UserCommonClass* cl = vm->upcalls->vmStackWalker; 

  while (i < real_size) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(ips[i++]);
    if (meth && meth->classDef == cl) {
      first = i;
      break;
    }   
  }

  return (jobject)recGetClassContext(vm, ips, real_size, first, 0);
}

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass _Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* Cl = (JavaObject*)_Cl;
  UserCommonClass* cl = (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField(Cl);
  return (jobject)cl->classLoader->getJavaClassLoader();
}
#endif

extern "C" JavaObject* getCallingClass() {
  UserClass* cl = JavaJIT::getCallingClass();
  if (cl) return cl->getClassDelegatee(JavaThread::get()->isolate);
  return 0;
}

extern "C" JavaObject* getCallingClassLoader() {
  return JavaJIT::getCallingClassLoader();
}

}
