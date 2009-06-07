//===------ ClasspathReflect.h - GNU classpath definitions of ----------------//
// java/lang/Class, java/lang/reflect/Field, java/lang/reflect/Method and ----//
// java/lang/reflect/Constructor as compiled by JnJVM. -----------------------//
//
//                              JnJVM
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

extern "C" jnjvm::JavaObject* internalFillInStackTrace(jnjvm::JavaObject*);
extern "C" void JavaObjectTracer(jnjvm::JavaObject*);

namespace jnjvm {

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
    JavaObjectTracer(obj);
    obj->pd->markAndTrace();
    obj->signers->markAndTrace();
    obj->constructor->markAndTrace();
    if (obj->vmdata) {
      JavaObject* Obj = obj->vmdata->classLoader->getJavaClassLoader();
      if (Obj) Obj->markAndTrace();
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
    JavaObjectTracer(obj);
    obj->name->markAndTrace();
    obj->declaringClass->markAndTrace();
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
    JavaObjectTracer(obj);
    obj->name->markAndTrace();
    obj->declaringClass->markAndTrace();
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
    JavaObjectTracer(obj);
    obj->clazz->markAndTrace();
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
  JavaObject* vmdata;

public:
  static void staticDestructor(JavaObjectVMThread* obj) {
    mvm::Thread* th = (mvm::Thread*)obj->vmdata;
    delete th;
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

  JavaObject* getReferent() const { return referent; }
  void setReferent(JavaObject* r) { referent = r; }
  
  static void staticTracer(JavaObjectReference* obj) {
    obj->queue->markAndTrace();
    obj->nextOnQueue->markAndTrace();
  }
};

}

#endif
