//===--ReferenceQueue.cpp - Implementation of soft/weak/phantom references-===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JavaReferenceQueue.h"

using namespace j3;

bool enqueueReference(gc* _obj) {
  JavaObject* obj = (JavaObject*)_obj;
  llvm_gcroot(obj, 0);
  llvm_gcroot(_obj, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = vm->upcalls->EnqueueReference;
  UserClass* cl = JavaObject::getClass(obj)->asClass();
  return (bool)meth->invokeIntSpecialBuf(vm, cl, obj, 0);
}

void Jnjvm::invokeEnqueueReference(gc* res) {
  llvm_gcroot(res, 0);
  TRY {
    enqueueReference(res);
  } IGNORE;
  vmkit::Thread::get()->clearException();
}

gc** Jnjvm::getObjectReferentPtr(gc* _obj) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  llvm_gcroot(obj, 0);
  llvm_gcroot(_obj, 0);
  return (gc**)JavaObjectReference::getReferentPtr(obj);
}

void Jnjvm::setObjectReferent(gc* _obj, gc* val) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  JavaObject *_val = (JavaObject*)val;
  llvm_gcroot(obj, 0);
  llvm_gcroot(_obj, 0);
  llvm_gcroot(val, 0);
  llvm_gcroot(_val, 0);
  JavaObjectReference::setReferent(obj, _val);
}
 
void Jnjvm::clearObjectReferent(gc* _obj) {
  JavaObjectReference* obj = (JavaObjectReference*)_obj;
  llvm_gcroot(obj, 0);
  llvm_gcroot(_obj, 0);
  JavaObjectReference::setReferent(obj, NULL);
}

typedef void (*destructor_t)(void*);

void invokeFinalizer(gc* _obj) {
  JavaObject* obj = (JavaObject*)_obj;
  llvm_gcroot(obj, 0);
  llvm_gcroot(_obj, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = vm->upcalls->FinalizeObject;
  UserClass* cl = JavaObject::getClass(obj)->asClass();
  meth->invokeIntVirtualBuf(vm, cl, obj, 0);
}

void invokeFinalize(gc* res) {
  llvm_gcroot(res, 0);
  TRY {
    invokeFinalizer(res);
  } IGNORE;
  vmkit::Thread::get()->clearException();
}

void Jnjvm::finalizeObject(gc* object) {
  JavaObject* res = (JavaObject*)object;
  llvm_gcroot(object, 0);
  llvm_gcroot(res, 0);
  JavaVirtualTable* VT = res->getVirtualTable();
  if (VT->operatorDelete) {
    destructor_t dest = (destructor_t)VT->destructor;
    dest(res);
  } else {
    invokeFinalize(res);
  }
}
