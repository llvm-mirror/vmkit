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

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_fillInStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject throwable) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
 
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  
  // Allocate the temporary data.
  std::vector<void*>* stack = new std::vector<void*>();

  // Get the frame context.
  th->getJavaFrameContext(*stack);
  
  // Set the tempory data in the new VMThrowable object.
  JavaObject* vmThrowable = vm->upcalls->newVMThrowable->doNew(vm);
  uint64 ptr = (uint64)vmThrowable + vm->upcalls->vmDataVMThrowable->ptrOffset;
  ((JavaObject**)ptr)[0] = (JavaObject*)stack;
  res = (jobject)vmThrowable;

  END_NATIVE_EXCEPTION

  return res;
}


static JavaObject* consStackElement(JavaMethod* meth, void* ip) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaObject* methodName = vm->internalUTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  const UTF8* internal = cl->name->internalToJava(vm, 0, cl->name->size);
  JavaObject* className = vm->UTF8ToStr(internal);
  JavaObject* sourceName = 0;
  
  Attribut* sourceAtt = cl->lookupAttribut(Attribut::sourceFileAttribut);
  
  // We don't have the bytes if the class was vmjc'ed.
  if (sourceAtt && cl->getBytes()) {
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

JNIEXPORT jobject JNICALL Java_java_lang_VMThrowable_getStackTrace(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject vmthrow, jobject throwable) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmDataVMThrowable;
  std::vector<void*>* stack = (std::vector<void*>*)
    field->getObjectField((JavaObject*)vmthrow);
  
  std::vector<void*>::iterator i = stack->begin(), e = stack->end();
  // remove the VMThrowable.fillInStackTrace method
  uint32 index = 1;
  ++i;
  while (i != e) {
    JavaMethod* meth = vm->IPToMethod<JavaMethod>(*i);
    assert(meth && "Wrong stack trace");
    if (meth->classDef->subclassOf(vm->upcalls->newThrowable)) {
      ++i;
      ++index;
    } else break;
  }

  ArrayObject* res = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(stack->size() - index, vm);
  
  index = 0;
  for (; i != e; ++i) {
    JavaMethod* meth = vm->IPToMethod<JavaMethod>(*i);
    assert(meth && "Wrong stack trace");
    res->elements[index++] = consStackElement(meth, *i);
  }
  
  delete stack;
  result = (jobject)res;

  END_NATIVE_EXCEPTION

  return result;
}

}

