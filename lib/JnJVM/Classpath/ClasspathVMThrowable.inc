//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "types.h"

#include "Classpath.h"
#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reader.h"

using namespace jnjvm;

extern "C" {

JavaObject* internalFillInStackTrace(JavaObject* throwable) {
  
  JavaObject* vmThrowable = 0;
  llvm_gcroot(throwable, 0);
  llvm_gcroot(vmThrowable, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  
  // Allocate the temporary data.
  std::vector<void*>* stack = new std::vector<void*>();

  // Get the frame context.
  th->getJavaFrameContext(*stack);
  
  // Set the tempory data in the new VMThrowable object.
  vmThrowable = vm->upcalls->newVMThrowable->doNew(vm);
  uint64 ptr = (uint64)vmThrowable + vm->upcalls->vmDataVMThrowable->ptrOffset;
  ((JavaObject**)ptr)[0] = (JavaObject*)stack;
  return vmThrowable;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* throwable) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(throwable, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  res = internalFillInStackTrace(throwable);

  END_NATIVE_EXCEPTION

  return res;
}


static JavaObject* consStackElement(JavaMethod* meth, void* ip) {

  JavaString* methodName = 0;
  JavaString* className = 0;
  JavaString* sourceName = 0;
  JavaObject* res = 0;
  llvm_gcroot(methodName, 0);
  llvm_gcroot(className, 0);
  llvm_gcroot(sourceName, 0);
  llvm_gcroot(res, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  methodName = vm->internalUTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  className = JavaString::internalToJava(cl->name, vm);
  
  Attribut* sourceAtt = cl->lookupAttribut(Attribut::sourceFileAttribut);
  
  // We don't have the bytes if the class was vmjc'ed.
  if (sourceAtt && cl->getBytes()) {
    Reader reader(sourceAtt, cl->getBytes());
    uint16 index = reader.readU2();
    sourceName = vm->internalUTF8ToStr(cl->getConstantPool()->UTF8At(index));
  }

  bool native = isNative(meth->access);

  UserClass* newS = vm->upcalls->newStackTraceElement;
  res = newS->doNew(vm);
  vm->upcalls->initStackTraceElement->invokeIntSpecial(vm, newS, res,
                                                       sourceName,
                                                       0, // source line
                                                       className,
                                                       methodName, native);
  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* vmthrow, JavaObject* throwable) {

  ArrayObject* result = 0;
  llvm_gcroot(vmthrow, 0);
  llvm_gcroot(throwable, 0);
  llvm_gcroot(result, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmDataVMThrowable;
  std::vector<void*>* stack = (std::vector<void*>*)
    field->getObjectField(vmthrow);
  
  std::vector<void*>::iterator i = stack->begin(), e = stack->end();
  // remove the VMThrowable.fillInStackTrace method
  uint32 index = 1;
  ++i;
  while (i != e) {
    JavaMethod* meth = vm->IPToMethod<JavaMethod>(*i);
    assert(meth && "Wrong stack trace");
    if (meth->classDef->isAssignableFrom(vm->upcalls->newThrowable)) {
      ++i;
      ++index;
    } else break;
  }

  result = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(stack->size() - index, vm);
  
  index = 0;
  for (; i != e; ++i) {
    JavaMethod* meth = vm->IPToMethod<JavaMethod>(*i);
    assert(meth && "Wrong stack trace");
    result->elements[index++] = consStackElement(meth, *i);
  }
  
  delete stack;

  END_NATIVE_EXCEPTION

  return result;
}

}

