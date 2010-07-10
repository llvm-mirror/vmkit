//===- ClasspathVMStackWalker.cpp -----------------------------------------===//
//===------------ GNU classpath gnu/classpath/VMStackWalker ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;

extern "C" {

JNIEXPORT JavaObject* JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {

  ArrayObject* result = NULL;
  JavaObject* delegatee = NULL;
  llvm_gcroot(result, 0);
  llvm_gcroot(delegatee, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  uint32 length = th->getFrameContextLength();
  
  uintptr_t* buffer = new uintptr_t[length];
  
  uint32 finalSize = th->getJavaFrameContext((void**)buffer);
  
  result = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(finalSize, vm);
   
  for (uint32 i = 0; i != finalSize; ++i) {
    JavaMethod* meth = ((JavaMethod**)buffer)[i];
    assert(meth && "Wrong stack trace");
    delegatee = meth->classDef->getClassDelegatee(vm);;
    ArrayObject::setElement(result, delegatee, i);
  }
  delete[] buffer;
  
  END_NATIVE_EXCEPTION
  
  return result;
}

JNIEXPORT JavaObject* JNICALL Java_gnu_classpath_VMStackWalker_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* Cl) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = cl->classLoader->getJavaClassLoader();

  END_NATIVE_EXCEPTION

  return res;
}

}
