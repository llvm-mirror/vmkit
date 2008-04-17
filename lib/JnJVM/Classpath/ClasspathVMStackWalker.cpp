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

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

#include <execinfo.h>

using namespace jnjvm;

extern "C" JavaMethod* ip_to_meth(int* ip);

extern "C" {

ArrayObject* recGetClassContext(int** stack, uint32 size, uint32 first, uint32 rec) {
  if (size != first) {
    JavaMethod* meth = ip_to_meth(stack[first]);
    if (meth) {
      ArrayObject* res = recGetClassContext(stack, size, first + 1, rec + 1); 
      res->setAt(rec, meth->classDef->getClassDelegatee());
      return res;
    } else {
      return recGetClassContext(stack, size, first + 1, rec);
    }   
  } else {
    Jnjvm* vm = JavaThread::get()->isolate;
    return ArrayObject::acons(rec, Classpath::classArrayClass, vm);
  }
}

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  int* ips[100];
  int real_size = backtrace((void**)(void*)ips, 100);
  int i = 0;
  int first = 0;
  CommonClass* cl = Classpath::vmStackWalker; 

  while (i < real_size) {
    JavaMethod* meth = ip_to_meth(ips[i++]);
    if (meth && meth->classDef == cl) {
      first = i;
      break;
    }   
  }

  return (jobject)recGetClassContext(ips, real_size, first, 0);
}

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass _Cl) {
  JavaObject* Cl = (JavaObject*)_Cl;
  CommonClass* cl = (CommonClass*)((*Cl)(Classpath::vmdataClass).PointerVal);
  return (jobject)cl->classLoader;
}

extern "C" JavaObject* getCallingClass() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 100);
  int n = 0;
  int i = 0;
  
  while (i < real_size) {
    JavaMethod* meth = ip_to_meth(ips[i++]);
    if (meth) {
      ++n;
      if (n == 1) return meth->classDef->getClassDelegatee();
    }   
  }

  return 0;
}

extern "C" JavaObject* getCallingClassLoader() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 100);
  int n = 0;
  int i = 0;
  
  while (i < real_size) {
    JavaMethod* meth = ip_to_meth(ips[i++]);
    if (meth) {
      ++n;
      if (n == 1) return meth->classDef->classLoader;
    }   
  }

  return 0;
}

}
