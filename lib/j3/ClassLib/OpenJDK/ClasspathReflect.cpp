//===- ClasspathReflect.cpp - Internal representation of core system classes -//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reflect.inc"

namespace j3 {

JavaMethod* JavaObjectConstructor::getInternalMethod(JavaObjectConstructor* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
  return &(cls->asClass()->virtualMethods[self->slot]);
}

JavaMethod* JavaObjectMethod::getInternalMethod(JavaObjectMethod* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
  return &(cls->asClass()->virtualMethods[self->slot]);
}


int JavaObjectThrowable::getStackTraceBase(JavaObjectThrowable * self) {
  JavaObject * stack = NULL;
  llvm_gcroot(self, 0);
  llvm_gcroot(stack, 0);

  if (!self->backtrace) return 0;

  Jnjvm* vm = JavaThread::get()->getJVM();

  stack = self->backtrace;
  sint32 index = 2;;
  while (index != JavaArray::getSize(stack)) {
    vmkit::FrameInfo* FI = vm->IPToFrameInfo(ArrayPtr::getElement((ArrayPtr*)stack, index));
    if (FI->Metadata == NULL) ++index;
    else {
      JavaMethod* meth = (JavaMethod*)FI->Metadata;
      assert(meth && "Wrong stack trace");
      if (meth->classDef->isSubclassOf(vm->upcalls->newThrowable)) {
        ++index;
      } else return index;
    }
  }

  assert(0 && "Invalid stack trace!");
  return 0;
}

int JavaObjectThrowable::getStackTraceDepth(JavaObjectThrowable * self) {
  JavaObject * stack = NULL;
  llvm_gcroot(self, 0);
  llvm_gcroot(stack, 0);

  if (!self->backtrace) return 0;

  Jnjvm* vm = JavaThread::get()->getJVM();

  stack = self->backtrace;
  sint32 index = getStackTraceBase(self);

  sint32 size = 0;
  sint32 cur = index;
  while (cur < JavaArray::getSize(stack)) {
    vmkit::FrameInfo* FI = vm->IPToFrameInfo(ArrayPtr::getElement((ArrayPtr*)stack, cur));
    ++cur;
    if (FI->Metadata != NULL) ++size;
  }

  return size;
}

JavaObjectConstructor* JavaObjectConstructor::createFromInternalConstructor(JavaMethod * cons, int i) {
  JavaObjectConstructor* ret = 0;
  JavaObject* pArr = 0;
  JavaObject* eArr = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(pArr, 0);
  llvm_gcroot(eArr, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader * classLoader = cons->classDef->classLoader;

  UserClass* Cons = vm->upcalls->newConstructor;
  pArr = cons->getParameterTypes(classLoader);
  eArr = cons->getExceptionTypes(classLoader);
  ret = (JavaObjectConstructor*)Cons->doNew(vm);
  JavaObject* const* Cl = cons->classDef->getDelegateePtr();
  vm->upcalls->initConstructor->invokeIntSpecial(vm, Cons, ret,
    Cl,           /* declaringClass */
    &pArr,        /* parameterTypes */
    &eArr,        /* checkedExceptions */
    cons->access, /* modifiers */
    i,            /* slot */
    NULL,         /* String signature */
    NULL,         /* annotations */
    NULL          /* parameterAnnotations */
  );

  return ret;
}

JavaObjectMethod* JavaObjectMethod::createFromInternalMethod(JavaMethod* meth, int i) {
  JavaObjectMethod* ret = 0;
  JavaString* str = 0;
  JavaObject* pArr = 0;
  JavaObject* eArr = 0;
  JavaObject* retTy = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(str, 0);
  llvm_gcroot(pArr, 0);
  llvm_gcroot(eArr, 0);
  llvm_gcroot(retTy, 0);

  // TODO: check parameter types
  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader * classLoader = meth->classDef->classLoader;

  UserClass* Meth = vm->upcalls->newMethod;
  ret = (JavaObjectMethod*)Meth->doNew(vm);
  str = vm->internalUTF8ToStr(meth->name);
  pArr = meth->getParameterTypes(classLoader);
  eArr = meth->getExceptionTypes(classLoader);
  retTy = meth->getReturnType(classLoader);
  JavaObject* const* Cl = meth->classDef->getClassDelegateePtr(vm);
  vm->upcalls->initMethod->invokeIntSpecial(vm, Meth, ret,
    Cl,           /* declaring class */
    &str,         /* name */
    &pArr,        /* parameter types */
    &retTy,       /* return type */
    &eArr,        /* exceptions */
    meth->access, /* modifiers */
    i,            /* slot */
    NULL,         /* signature */
    NULL,         /* annotations */
    NULL,         /* parameter annotations */
    NULL);        /* default annotations */

  return ret;
}

JavaObjectField* JavaObjectField::createFromInternalField(JavaField* field, int i) {
  JavaObjectField* ret = 0;
  JavaString* name = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(name, 0);

  // TODO: check parameter types
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* Field = vm->upcalls->newField;
  ret = (JavaObjectField*)Field->doNew(vm);
  name = vm->internalUTF8ToStr(field->name);

  //type->Class
  JnjvmClassLoader* loader = field->classDef->classLoader;
  UserCommonClass * fieldCl = field->getSignature()->assocClass(loader);
  assert(fieldCl);
  JavaObject* const* type = fieldCl->getClassDelegateePtr(vm);
  JavaObject* const* Cl = field->classDef->getClassDelegateePtr(vm);

  // TODO:Implement these!
  JavaObject** sign = NULL;
  JavaObject** annArray = NULL;

  /* java.reflect.Field(
  *   Class declaringClass,
  *   String name,
  *   Class type,
  *   int modifiers,
  *   int slot,
  *   String signature,
  *   byte[] annotations)
  */
  vm->upcalls->initField->invokeIntSpecial(vm, Field, ret,
    Cl,
    &name,
    type,
    field->access,
    i,
    sign,
    annArray);

  return ret;
}

} // end namespace j3
