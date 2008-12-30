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

#include "ClasspathReflect.h"
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

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  std::vector<void*> stack;
  
  th->getJavaFrameContext(stack);
  
  ArrayObject* res = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(stack.size(), vm);

  std::vector<void*>::iterator i = stack.begin(), e = stack.end();
  uint32 index = 0;
 
  for (; i != e; ++i) {
    JavaMethod* meth = vm->IPToJavaMethod(*i);
    assert(meth && "Wrong stack trace");
    res->elements[index++] = meth->classDef->getClassDelegatee(vm);
  }
  
  result = (jobject)res;
  
  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jobject JNICALL Java_gnu_classpath_VMStackWalker_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass _Cl) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  JavaObject* Cl = (JavaObject*)_Cl;
  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  res = (jobject)cl->classLoader->getJavaClassLoader();

  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* getCallingClass() {
  
  JavaObject* res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  UserClass* cl = th->getCallingClass();
  if (cl) res = cl->getClassDelegatee(th->getJVM());

  END_NATIVE_EXCEPTION

  return res;
}

extern "C" JavaObject* getCallingClassLoader() {
  
  JavaObject *res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)
  JavaThread* th = JavaThread::get();
  UserClass* cl = th->getCallingClass();
  if (cl) res = cl->classLoader->getJavaClassLoader();  
  END_NATIVE_EXCEPTION

  return res;
}

}
