//===----------- JavaObject.cpp - Java object definition ------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Locks.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

#include <jni.h>
#include "debug.h"

using namespace j3;

static const int hashCodeIncrement = mvm::GCBitMask + 1;
uint16_t JavaObject::hashCodeGenerator = hashCodeIncrement;

/// hashCode - Return the hash code of this object.
uint32_t JavaObject::hashCode(JavaObject* self) {
  llvm_gcroot(self, 0);
  if (!mvm::MovesObject) return (uint32_t)(long)self;

  uintptr_t header = self->header;
  uintptr_t GCBits = header & mvm::GCBitMask;
  uintptr_t val = header & mvm::HashMask;
  if (val != 0) {
    return val ^ (uintptr_t)getClass(self);
  }
  val = hashCodeGenerator;
  hashCodeGenerator += hashCodeIncrement;
  val = val % mvm::HashMask;
  if (val == 0) {
    // It is possible that in the same time, a thread is in this method and
    // gets the same hash code value than this thread. This is fine.
    val = hashCodeIncrement;
    hashCodeGenerator += hashCodeIncrement;
  }
  assert(val > mvm::GCBitMask);
  assert(val <= mvm::HashMask);

  do {
    header = self->header;
    if ((header & mvm::HashMask) != 0) break;
    uintptr_t newHeader = header | val;
    assert((newHeader & ~mvm::HashMask) == header);
    __sync_val_compare_and_swap(&(self->header), header, newHeader);
  } while (true);

  assert((self->header & mvm::HashMask) != 0);
  assert(GCBits == (self->header & mvm::GCBitMask));
  return (self->header & mvm::HashMask) ^ (uintptr_t)getClass(self);
}


void JavaObject::waitIntern(
    JavaObject* self, struct timeval* info, bool timed) {
  llvm_gcroot(self, 0);
  JavaThread* thread = JavaThread::get();
  mvm::LockSystem& table = thread->getJVM()->lockSystem;

  if (!owner(self)) {
    thread->getJVM()->illegalMonitorStateException(self);
    UNREACHABLE();
  }

  bool interrupted = thread->lockingThread.wait(self, table, info, timed);

  if (interrupted) {
    thread->getJVM()->interruptedException(self);
    UNREACHABLE();
  }
}

void JavaObject::wait(JavaObject* self) {
  llvm_gcroot(self, 0);
  waitIntern(self, NULL, false);
}

void JavaObject::timedWait(JavaObject* self, struct timeval& info) {
  llvm_gcroot(self, 0);
  waitIntern(self, &info, true);
}

void JavaObject::notify(JavaObject* self) {
  llvm_gcroot(self, 0);
  JavaThread* thread = JavaThread::get();
  mvm::LockSystem& table = thread->getJVM()->lockSystem;

  if (!owner(self)) {
    thread->getJVM()->illegalMonitorStateException(self);
    UNREACHABLE();
  }
  thread->lockingThread.notify(self, table);
}

void JavaObject::notifyAll(JavaObject* self) {
  llvm_gcroot(self, 0);
  JavaThread* thread = JavaThread::get();
  mvm::LockSystem& table = thread->getJVM()->lockSystem;

  if (!owner(self)) {
    thread->getJVM()->illegalMonitorStateException(self);
    UNREACHABLE();
  }
  thread->lockingThread.notifyAll(self, table);
}

void JavaObject::overflowThinLock(JavaObject* self) {
  llvm_gcroot(self, 0);
  mvm::ThinLock::overflowThinLock(self, JavaThread::get()->getJVM()->lockSystem);
}

void JavaObject::acquire(JavaObject* self) {
  llvm_gcroot(self, 0);
  mvm::ThinLock::acquire(self, JavaThread::get()->getJVM()->lockSystem);
}

void JavaObject::release(JavaObject* self) {
  llvm_gcroot(self, 0);
  mvm::ThinLock::release(self, JavaThread::get()->getJVM()->lockSystem);
}

bool JavaObject::owner(JavaObject* self) {
  llvm_gcroot(self, 0);
  return mvm::ThinLock::owner(self, JavaThread::get()->getJVM()->lockSystem);
}

void JavaObject::decapsulePrimitive(JavaObject* obj, Jnjvm *vm, jvalue* buf,
                                    const Typedef* signature) {

  llvm_gcroot(obj, 0);

  if (!signature->isPrimitive()) {
    if (obj && !(getClass(obj)->isOfTypeName(signature->getName()))) {
      vm->illegalArgumentException("wrong type argument");
    }
    return;
  } else if (obj == NULL) {
    vm->illegalArgumentException("");
  } else {
    UserCommonClass* cl = getClass(obj);
    UserClassPrimitive* value = cl->toPrimitive(vm);
    const PrimitiveTypedef* prim = (const PrimitiveTypedef*)signature;

    if (value == 0) {
      vm->illegalArgumentException("");
    }
    
    if (prim->isShort()) {
      if (value == vm->upcalls->OfShort) {
        (*buf).s = vm->upcalls->shortValue->getInstanceInt16Field(obj);
        return;
      } else if (value == vm->upcalls->OfByte) {
        (*buf).s = (sint16)vm->upcalls->byteValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isByte()) {
      if (value == vm->upcalls->OfByte) {
        (*buf).b = vm->upcalls->byteValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isBool()) {
      if (value == vm->upcalls->OfBool) {
        (*buf).z = vm->upcalls->boolValue->getInstanceInt8Field(obj);
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isInt()) {
      if (value == vm->upcalls->OfInt) {
        (*buf).i = vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).i = (sint32)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).i = (uint32)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).i = (sint32)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isChar()) {
      if (value == vm->upcalls->OfChar) {
        (*buf).c = (uint16)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isFloat()) {
      if (value == vm->upcalls->OfFloat) {
        (*buf).f = (float)vm->upcalls->floatValue->getInstanceFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).f = (float)(sint32)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).f = (float)(uint32)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).f = (float)(sint32)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).f = (float)(sint32)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).f = (float)vm->upcalls->longValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isDouble()) {
      if (value == vm->upcalls->OfDouble) {
        (*buf).d = (double)vm->upcalls->doubleValue->getInstanceDoubleField(obj);
      } else if (value == vm->upcalls->OfFloat) {
        (*buf).d = (double)vm->upcalls->floatValue->getInstanceFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        (*buf).d = (double)(sint64)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).d = (double)(uint64)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).d = (double)(sint16)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).d = (double)(sint32)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).d = (double)(sint64)vm->upcalls->longValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    } else if (prim->isLong()) {
      if (value == vm->upcalls->OfByte) {
        (*buf).j = (sint64)vm->upcalls->byteValue->getInstanceInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        (*buf).j = (sint64)(uint64)vm->upcalls->charValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        (*buf).j = (sint64)vm->upcalls->shortValue->getInstanceInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        (*buf).j = (sint64)vm->upcalls->intValue->getInstanceInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        (*buf).j = (sint64)vm->upcalls->intValue->getInstanceLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      return;
    }
  }
  // can not be here
  return;
}

bool JavaObject::instanceOf(JavaObject* self, UserCommonClass* cl) {
  llvm_gcroot(self, 0);
  if (self == NULL) return false;
  else return getClass(self)->isAssignableFrom(cl);
}
