//===-- ClasspathReflect.h - Internal representation of core system classes --//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_CLASSPATH_REFLECT_H
#define JNJVM_CLASSPATH_REFLECT_H

#include "MvmGC.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include <JavaThread.h>

extern "C" j3::JavaObject* internalFillInStackTrace(j3::JavaObject*);

namespace j3 {

class JavaObjectClass : public JavaObject {
private:
  JavaObject* signers;
  JavaObject* pd;
  UserCommonClass* vmdata;
  JavaObject* constructor;

public:
  
  static UserCommonClass* getClass(JavaObjectClass* cl) {
    llvm_gcroot(cl, 0);
    return cl->vmdata;
  }

  static void setClass(JavaObjectClass* cl, UserCommonClass* vmdata) {
    llvm_gcroot(cl, 0);
    cl->vmdata = vmdata;
  }

  static void setProtectionDomain(JavaObjectClass* cl, JavaObject* pd) {
    llvm_gcroot(cl, 0);
    llvm_gcroot(pd, 0);
    cl->pd = pd;
  }
  
  static JavaObject* getProtectionDomain(JavaObjectClass* cl) {
    llvm_gcroot(cl, 0);
    return cl->pd;
  }

  static void staticTracer(JavaObjectClass* obj, uintptr_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->pd, closure);
    mvm::Collector::markAndTrace(obj, &obj->signers, closure);
    mvm::Collector::markAndTrace(obj, &obj->constructor, closure);
    if (obj->vmdata) {
      JavaObject** Obj = obj->vmdata->classLoader->getJavaClassLoaderPtr();
      if (*Obj) mvm::Collector::markAndTraceRoot(Obj, closure);
    }
  }
};

class JavaObjectField : public JavaObject {
private:
  uint8 flag;
  JavaObjectClass* declaringClass;
  JavaObject* name;
  uint32 slot;

public:

  static void staticTracer(JavaObjectField* obj, uintptr_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->name, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaringClass, closure);
  }

  static JavaField* getInternalField(JavaObjectField* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return &(cls->asClass()->virtualFields[self->slot]);
  }

  static UserClass* getClass(JavaObjectField* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return cls->asClass();
  }

};

class JavaObjectMethod : public JavaObject {
private:
  uint8 flag;
  JavaObjectClass* declaringClass;
  JavaObject* name;
  uint32 slot;

public:
  
  static void staticTracer(JavaObjectMethod* obj, uintptr_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->name, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaringClass, closure);
  }
  
  static JavaMethod* getInternalMethod(JavaObjectMethod* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return &(cls->asClass()->virtualMethods[self->slot]);
  }
  
  static UserClass* getClass(JavaObjectMethod* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return cls->asClass();
  }

};

class JavaObjectConstructor : public JavaObject {
private:
  uint8 flag;
  JavaObjectClass* declaringClass;
  uint32 slot;

public:
  static void staticTracer(JavaObjectConstructor* obj, uintptr_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->declaringClass, closure);
  }
  
  static JavaMethod* getInternalMethod(JavaObjectConstructor* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return &(cls->asClass()->virtualMethods[self->slot]);
  }
  
  static UserClass* getClass(JavaObjectConstructor* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->declaringClass); 
    return cls->asClass();
  }

};

class JavaObjectVMThread : public JavaObject {
private:
  JavaObject* thread;
  bool running;
  JavaThread* vmdata;

public:
  static void staticDestructor(JavaObjectVMThread* obj) {
    llvm_gcroot(obj, 0);
    mvm::Thread::releaseThread(obj->vmdata->mut);
  }
  
  static void staticTracer(JavaObjectVMThread* obj, uintptr_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->thread, closure);
  }

  static void setVmdata(JavaObjectVMThread* vmthread,
                        JavaThread* internal_thread) {
    llvm_gcroot(vmthread, 0);
    vmthread->vmdata = internal_thread;
  }

};


class JavaObjectThrowable : public JavaObject {
private:
  JavaObject* detailedMessage;
  JavaObject* cause;
  JavaObject* stackTrace;
  JavaObject* vmState;

public:

  static void setDetailedMessage(JavaObjectThrowable* self, JavaObject* obj) {
    llvm_gcroot(self, 0);
    llvm_gcroot(obj, 0);
    self->detailedMessage = obj;
  }

  static void fillInStackTrace(JavaObjectThrowable* self) {
    llvm_gcroot(self, 0);
    self->cause = self;
    self->vmState = internalFillInStackTrace(self);
    self->stackTrace = NULL;
  }
};

class JavaObjectReference : public JavaObject {
private:
  JavaObject* referent;
  JavaObject* queue;
  JavaObject* nextOnQueue;

public:
  static void init(JavaObjectReference* self, JavaObject* r, JavaObject* q) {
    llvm_gcroot(self, 0);
    llvm_gcroot(r, 0);
    llvm_gcroot(q, 0);
    self->referent = r;
    self->queue = q;
  }

  static JavaObject** getReferentPtr(JavaObjectReference* self) {
    llvm_gcroot(self, 0);
    return &(self->referent);
  }

  static void setReferent(JavaObjectReference* self, JavaObject* r) {
    llvm_gcroot(self, 0);
    llvm_gcroot(r, 0);
    self->referent = r;
  }
};

}

#endif
