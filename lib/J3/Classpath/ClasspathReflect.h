//===------ ClasspathReflect.h - GNU classpath definitions of ----------------//
// java/lang/Class, java/lang/reflect/Field, java/lang/reflect/Method and ----//
// java/lang/reflect/Constructor as compiled by JnJVM. -----------------------//
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

#include <JavaClass.h>
#include <JavaObject.h>

extern "C" j3::JavaObject* internalFillInStackTrace(j3::JavaObject*);

namespace j3 {

class JavaObjectClass : public JavaObject {
public:
  JavaObject* signers;
  JavaObject* pd;
  UserCommonClass* vmdata;
  JavaObject* constructor;

public:
  
  UserCommonClass* getClass() {
    return vmdata;
  }

  static void staticTracer(JavaObjectClass* obj) {
    mvm::Collector::markAndTrace(obj, &obj->pd);
    mvm::Collector::markAndTrace(obj, &obj->signers);
    mvm::Collector::markAndTrace(obj, &obj->constructor);
    if (obj->vmdata) {
      JavaObject** Obj = obj->vmdata->classLoader->getJavaClassLoaderPtr();
      if (*Obj) mvm::Collector::markAndTraceRoot(Obj);
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

  static void staticTracer(JavaObjectField* obj) {
    mvm::Collector::markAndTrace(obj, &obj->name);
    mvm::Collector::markAndTrace(obj, &obj->declaringClass);
  }

  JavaField* getInternalField() {
    return &(((UserClass*)declaringClass->vmdata)->virtualFields[slot]);
  }

  UserClass* getClass() {
    return declaringClass->vmdata->asClass();
  }

};

class JavaObjectMethod : public JavaObject {
private:
  uint8 flag;
  JavaObjectClass* declaringClass;
  JavaObject* name;
  uint32 slot;

public:
  
  static void staticTracer(JavaObjectMethod* obj) {
    mvm::Collector::markAndTrace(obj, &obj->name);
    mvm::Collector::markAndTrace(obj, &obj->declaringClass);
  }
  
  JavaMethod* getInternalMethod() {
    return &(((UserClass*)declaringClass->vmdata)->virtualMethods[slot]);
  }
  
  UserClass* getClass() {
    return declaringClass->vmdata->asClass();
  }

};

class JavaObjectConstructor : public JavaObject {
private:
  uint8 flag;
  JavaObjectClass* clazz;
  uint32 slot;

public:
  static void staticTracer(JavaObjectConstructor* obj) {
    mvm::Collector::markAndTrace(obj, &obj->clazz);
  }
  
  JavaMethod* getInternalMethod() {
    return &(((UserClass*)clazz->vmdata)->virtualMethods[slot]);
  }
  
  UserClass* getClass() {
    return clazz->vmdata->asClass();
  }

};

class JavaObjectVMThread : public JavaObject {
private:
  JavaObject* thread;
  bool running;
  void* vmdata;

public:
  static void staticDestructor(JavaObjectVMThread* obj) {
    mvm::Thread::releaseThread(obj->vmdata);
  }
  
  static void staticTracer(JavaObjectVMThread* obj) {
    mvm::Collector::markAndTrace(obj, &obj->thread);
  }

};


class JavaObjectThrowable : public JavaObject {
private:
  JavaObject* detailedMessage;
  JavaObject* cause;
  JavaObject* stackTrace;
  JavaObject* vmState;

public:

  void setDetailedMessage(JavaObject* obj) {
    detailedMessage = obj;
  }

  void fillInStackTrace() {
    cause = this;
    vmState = internalFillInStackTrace(this);
    stackTrace = 0;
  }
};

class JavaObjectReference : public JavaObject {
private:
  JavaObject* referent;
  JavaObject* queue;
  JavaObject* nextOnQueue;

public:
  void init(JavaObject* r, JavaObject* q) {
    referent = r;
    queue = q;
  }

  JavaObject** getReferentPtr() { return &referent; }
  void setReferent(JavaObject* r) { referent = r; }
  
  static void staticTracer(JavaObjectReference* obj) {
    mvm::Collector::markAndTrace(obj, &obj->queue);
    mvm::Collector::markAndTrace(obj, &obj->nextOnQueue);
  }
};

}

#endif
