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
  std::vector<void*> stack;

  // Get the frame context.
  th->getFrameContext(stack);
  
  ClassArray* cl = sizeof(void*) == 4 ? vm->upcalls->ArrayOfInt : 
                                        vm->upcalls->ArrayOfLong;
  JavaArray* result = (JavaArray*) cl->doNew(stack.size(), vm);
  void** tab = (void**)result->elements;
  uint32 index = 0;
  for (std::vector<void*>::iterator i = stack.begin(), e = stack.end();
       i != e; ++i, ++index) {
    tab[index] = *i;
  }
  
  // Set the tempory data in the new VMThrowable object.
  vmThrowable = vm->upcalls->newVMThrowable->doNew(vm);
  uint64 ptr = (uint64)vmThrowable + vm->upcalls->vmDataVMThrowable->ptrOffset;
  ((JavaObject**)ptr)[0] = result;
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
    Reader reader(sourceAtt, cl->getBytesPtr());
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
  JavaArray* stack = (JavaArray*)field->getObjectField(vmthrow);
  
  // remove the VMThrowable.fillInStackTrace method
  sint32 index = 1;;
  while (index != stack->size) {
    mvm::MethodInfo* MI = vm->IPToMethodInfo(stack->elements[index]);
    if (MI->MethodType != 1) ++index;
    else {
      JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();
      assert(meth && "Wrong stack trace");
      if (meth->classDef->isAssignableFrom(vm->upcalls->newThrowable)) {
        ++index;
      } else break;
    }
  }
  
  sint32 size = 0;
  sint32 cur = index;
  while (cur < stack->size) {
    mvm::MethodInfo* MI = vm->IPToMethodInfo(stack->elements[cur]);
    ++cur;
    if (MI->MethodType == 1) ++size;
  }

  result = (ArrayObject*)
    vm->upcalls->stackTraceArray->doNew(size, vm);
  
  cur = 0;
  for (sint32 i = index; i < stack->size; ++i) {
    mvm::MethodInfo* MI = vm->IPToMethodInfo(stack->elements[i]);
    if (MI->MethodType == 1) {
      JavaMethod* meth = (JavaMethod*)MI->getMetaInfo();
      result->elements[cur] = consStackElement(meth, stack->elements[i]);
      cur++;
    }
  }
  
  END_NATIVE_EXCEPTION

  return result;
}

}

