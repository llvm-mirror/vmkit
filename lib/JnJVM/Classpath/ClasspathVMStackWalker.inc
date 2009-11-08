//===- ClasspathVMStackWalker.cpp -----------------------------------------===//
//===------------ GNU classpath gnu/classpath/VMStackWalker ---------------===//
//
//                              JnJVM
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

using namespace jnjvm;

extern "C" {

JNIEXPORT JavaObject* JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {

  ArrayObject* result = 0;
  llvm_gcroot(result, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  std::vector<JavaMethod*> stack;
  
  th->getJavaFrameContext(stack);
  
  result = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(stack.size(), vm);

  std::vector<JavaMethod*>::iterator i = stack.begin(), e = stack.end();
  uint32 index = 0;
 
  for (; i != e; ++i) {
    JavaMethod* meth = *i;
    assert(meth && "Wrong stack trace");
    result->elements[index++] = meth->classDef->getClassDelegatee(vm);
  }
  
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
