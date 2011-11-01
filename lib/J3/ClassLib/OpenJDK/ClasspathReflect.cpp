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
#include "Jnjvm.h"
#include "JavaUpcalls.h"
#include "JavaArray.h"

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
    mvm::FrameInfo* FI = vm->IPToFrameInfo(ArrayPtr::getElement((ArrayPtr*)stack, index));
    if (FI->Metadata == NULL) ++index;
    else {
      JavaMethod* meth = (JavaMethod*)FI->Metadata;
      assert(meth && "Wrong stack trace");
      if (meth->classDef->isAssignableFrom(vm->upcalls->newThrowable)) {
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
    mvm::FrameInfo* FI = vm->IPToFrameInfo(ArrayPtr::getElement((ArrayPtr*)stack, cur));
    ++cur;
    if (FI->Metadata != NULL) ++size;
  }

  return size;
}

}
