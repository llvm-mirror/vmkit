//===- ClasspathReflect.cpp - Internal representation of core system classes -//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"

#include "Reflect.inc"

namespace j3 {

JavaMethod* JavaObjectConstructor::getInternalMethod(JavaObjectConstructor* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
  return &(cls->asClass()->virtualMethods[self->slot]);
}


JavaMethod* JavaObjectMethod::getInternalMethod(JavaObjectMethod* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
  return &(cls->asClass()->virtualMethods[self->slot]);
}

JavaObjectConstructor* JavaObjectConstructor::createFromInternalConstructor(JavaMethod * cons, int i) {
  JavaObjectConstructor* ret = 0;
  llvm_gcroot(ret, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* Cons = vm->upcalls->newConstructor;
  ret = (JavaObjectConstructor*)Cons->doNew(vm);
  JavaObject* const* Cl = cons->classDef->getDelegateePtr();
  vm->upcalls->initConstructor->invokeIntSpecial(vm, Cons, ret,
    Cl, i);

  return ret;
}

JavaObjectMethod* JavaObjectMethod::createFromInternalMethod(JavaMethod* meth, int i) {
  JavaObjectMethod* ret = 0;
  JavaString* str = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(str, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* Meth = vm->upcalls->newMethod;
  ret = (JavaObjectMethod*)Meth->doNew(vm);
  str = vm->internalUTF8ToStr(meth->name);
  JavaObject* const* Cl = meth->classDef->getDelegateePtr();
  vm->upcalls->initMethod->invokeIntSpecial(vm, Meth, ret,
    Cl, &str, i);

  return ret;
}

JavaObjectField* JavaObjectField::createFromInternalField(JavaField* field, int i) {
  JavaObjectField* ret = 0;
  JavaString* name = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(name, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* Field = vm->upcalls->newField;
  ret = (JavaObjectField*)Field->doNew(vm);
  name = vm->internalUTF8ToStr(field->name);
  JavaObject* const* Cl = field->classDef->getDelegateePtr();
  vm->upcalls->initField->invokeIntSpecial(vm, Field, ret,
    Cl, &name, i);

  return ret;
}
}
