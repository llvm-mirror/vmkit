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

#ifdef ISOLATE_SHARING
uint32 getPools(UserConstantPool** pools, uint32 size) {
  unsigned int* top;
  register unsigned int  **cur = &top;
  register unsigned int  **max = (unsigned int**)mvm::Thread::get()->baseSP;
  
  uint32 i = 0;
  for(; cur<max && i < size; cur++) {
    void* obj = (void*)(*cur);
    obj = Collector::begOf(obj);
    if (obj && ((mvm::Object*)obj)->getVirtualTable() == UserConstantPool::VT) {
      UserConstantPool* ctp = (UserConstantPool*)obj;
      pools[i++] = ctp;
    }
  }
  return i;
}
#endif

#ifdef ISOLATE_SHARING
JavaObject* getClassInContext(Jnjvm* vm, Class* cl, UserConstantPool** ctps, uint32& ctpIndex) {
  for (; ctpIndex < 100; ++ctpIndex) {
    UserClass* newCl = ctps[ctpIndex]->getClass();
    if (cl == newCl->classDef) return newCl->getClassDelegatee(vm);
  }
  return 0;
}

ArrayObject* recGetClassContext(Jnjvm* vm, int** stack, uint32 size, uint32 first, uint32 rec, UserConstantPool** ctps, uint32 ctpIndex) {
  if (size != first) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(stack[first]);
    if (meth) {
      JavaObject* obj = getClassInContext(vm, meth->classDef, ctps, ctpIndex);
      ArrayObject* res = recGetClassContext(vm, stack, size, first + 1, rec + 1, ctps, ctpIndex); 
      res->elements[rec] = obj;
      assert(res->elements[rec] && "Did not found the user class");
      return res;
    } else {
      return recGetClassContext(vm, stack, size, first + 1, rec, ctps, ctpIndex);
    }   
  } else {
    return (ArrayObject*)vm->upcalls->classArrayClass->doNew(rec, vm);
  }
}
#else
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
    return (ArrayObject*)vm->upcalls->classArrayClass->doNew(rec, vm);
  }
}
#endif

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  Jnjvm* vm = JavaThread::get()->isolate;
  int* ips[100];
  int real_size = mvm::jit::getBacktrace((void**)(void*)ips, 100);
#ifdef ISOLATE_SHARING
  UserConstantPool* pools[100];
  getPools(pools, 100);
#endif

  int i = 0;
  int first = 0;
  UserCommonClass* cl = vm->upcalls->vmStackWalker; 

  while (i < real_size) {
    JavaMethod* meth = JavaJIT::IPToJavaMethod(ips[i++]);
#ifdef ISOLATE_SHARING
    if (meth && meth->classDef == cl->classDef) {
#else
    if (meth && meth->classDef == cl) {
#endif
      first = i;
      break;
    }   
  }

#ifdef ISOLATE_SHARING
  return (jobject)recGetClassContext(vm, ips, real_size, first, 0, pools, 0);
#else
  return (jobject)recGetClassContext(vm, ips, real_size, first, 0);
#endif
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

extern "C" JavaObject* getCallingClass() {
  UserClass* cl = JavaJIT::getCallingClass();
  if (cl) return cl->getClassDelegatee(JavaThread::get()->isolate);
  return 0;
}

extern "C" JavaObject* getCallingClassLoader() {
  return JavaJIT::getCallingClassLoader();
}

}
