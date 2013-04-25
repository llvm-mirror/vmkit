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


JavaMethod* JavaObjectVMConstructor::getInternalMethod(JavaObjectVMConstructor* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass);
  return &(cls->asClass()->virtualMethods[self->slot]);
}

JavaMethod* JavaObjectConstructor::getInternalMethod(JavaObjectConstructor* self) {
  llvm_gcroot(self, 0);
  return JavaObjectVMConstructor::getInternalMethod(self->vmCons);
}


JavaMethod* JavaObjectVMMethod::getInternalMethod(JavaObjectVMMethod* self) {
  llvm_gcroot(self, 0);
  UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass);
  return &(cls->asClass()->virtualMethods[self->slot]);
}

JavaMethod* JavaObjectMethod::getInternalMethod(JavaObjectMethod* self) {
  llvm_gcroot(self, 0);
  return JavaObjectVMMethod::getInternalMethod(self->vmMethod);
}

JavaObjectConstructor* JavaObjectConstructor::createFromInternalConstructor(JavaMethod * cons, int i) {
  JavaObjectConstructor* ret = 0;
  JavaObject* vmConsInstance = 0;
  UserClass* VMConsClass = 0;
  UserClass* ConstructorClass = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(cons, 0);
  llvm_gcroot(vmConsInstance, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  VMConsClass = vm->upcalls->newVMConstructor;
  vmConsInstance = VMConsClass->doNew(vm);
  JavaObject* const* Cl = cons->classDef->getDelegateePtr();
  vm->upcalls->initVMConstructor->invokeIntSpecial(vm, VMConsClass, vmConsInstance, Cl, i);
  ConstructorClass = vm->upcalls->newConstructor;
  ret = (JavaObjectConstructor*)ConstructorClass->doNew(vm);
  vm->upcalls->initConstructor->invokeIntSpecial(vm, ConstructorClass, ret, &vmConsInstance);
  return ret;
}

JavaObjectMethod* JavaObjectMethod::createFromInternalMethod(JavaMethod* meth, int i) {
  JavaObjectMethod* ret = 0;
  JavaObjectVMMethod* vmMeth = 0;
  JavaString* str = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(str, 0);
  llvm_gcroot(vmMeth, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();

  UserClass* VMMeth = vm->upcalls->newVMMethod;
  vmMeth = (JavaObjectVMMethod*)VMMeth->doNew(vm);


  str = vm->internalUTF8ToStr(meth->name);
  JavaObject* const* Cl = meth->classDef->getDelegateePtr();

  vm->upcalls->initVMMethod->invokeIntSpecial(vm, VMMeth, vmMeth, Cl, &str, i);

  //vmMeth->name = str;
  //vmMeth->declaringClass = (JavaObject*)Cl; // I don't like this
  //vmMeth->slot = i;

  UserClass* Meth = vm->upcalls->newMethod;
  ret = (JavaObjectMethod*)Meth->doNew(vm);

  vm->upcalls->initMethod->invokeIntSpecial(vm, Meth, ret,&vmMeth);
  return ret;
}

JavaObjectField* JavaObjectField::createFromInternalField(JavaField* field, int i) {
  JavaObjectField* ret = 0;
  JavaString* name = 0;
  JavaObject* vmField = 0;

  llvm_gcroot(ret, 0);
  llvm_gcroot(name, 0);
  llvm_gcroot(vmField, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* VMFieldClass = vm->upcalls->newVMField;
  vmField = VMFieldClass->doNew(vm);
  name = vm->internalUTF8ToStr(field->name);
  JavaObject* const* Cl = field->classDef->getDelegateePtr();
  vm->upcalls->initVMField->invokeIntSpecial(vm, VMFieldClass, vmField,Cl,&name, i);
  UserClass* FieldClass = vm->upcalls->newField;
  ret = (JavaObjectField*)FieldClass->doNew(vm);
  vm->upcalls->initField->invokeIntSpecial(vm, FieldClass, ret, &vmField);
  return ret;
}
}
