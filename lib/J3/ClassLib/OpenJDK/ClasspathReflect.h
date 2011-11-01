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
#include "JavaThread.h"
#include "JavaString.h"

extern "C" j3::JavaObject* internalFillInStackTrace(j3::JavaObject*);
namespace j3 {

class JavaObjectClass : public JavaObject {
private:
  JavaObject* cachedConstructor;
  JavaObject* newInstanceCallerCache;
  JavaString* name;
  JavaObject* declaredFields;
  JavaObject* publicFields;
  JavaObject* declaredMethods;
  JavaObject* publicMethods;
  JavaObject* declaredConstructors;
  JavaObject* publicConstructors;
  JavaObject* declaredPublicFields;
  uint32_t classRedefinedCount;
  uint32_t lastRedefinedCount;
  JavaObject* genericInfo;
  JavaObject* enumConstants;
  JavaObject* enumConstantDictionary;
  JavaObject* annotations;
  JavaObject* declaredAnnotations;
  JavaObject* annotationType;
  // Extra fields added by VM
  UserCommonClass * internalClass;
  JavaObject * pd;

public:

  static UserCommonClass* getClass(JavaObjectClass* cl) {
    llvm_gcroot(cl, 0);
    return cl->internalClass;
  }

  static void setClass(JavaObjectClass* cl, UserCommonClass* vmdata) {
    llvm_gcroot(cl, 0);
    cl->internalClass = vmdata;
  }

  static void setProtectionDomain(JavaObjectClass* cl, JavaObject* pd) {
    llvm_gcroot(cl, 0);
    llvm_gcroot(pd, 0);
    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)cl, (gc**)&(cl->pd), (gc*)pd);
  }

  static JavaObject* getProtectionDomain(JavaObjectClass* cl) {
    llvm_gcroot(cl, 0);
    return cl->pd;
  }

  static void staticTracer(JavaObjectClass* obj, word_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->cachedConstructor, closure);
    mvm::Collector::markAndTrace(obj, &obj->newInstanceCallerCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->name, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredFields, closure);
    mvm::Collector::markAndTrace(obj, &obj->publicFields, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredMethods, closure);
    mvm::Collector::markAndTrace(obj, &obj->publicMethods, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredConstructors, closure);
    mvm::Collector::markAndTrace(obj, &obj->publicConstructors, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredPublicFields, closure);
    mvm::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    mvm::Collector::markAndTrace(obj, &obj->enumConstants, closure);
    mvm::Collector::markAndTrace(obj, &obj->enumConstantDictionary, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotationType, closure);

    UserCommonClass * cl = getClass(obj);
    if (cl) {
      JavaObject** Obj = cl->classLoader->getJavaClassLoaderPtr();
      if (*Obj) mvm::Collector::markAndTraceRoot(Obj, closure);
    }
  }
};

class JavaObjectField : public JavaObject {
private:
  // AccessibleObject fields
  uint8 override;
  // Field fields
  JavaObjectClass * clazz;
  uint32_t slot;
  JavaObject* name;
  JavaObject* type;
  uint32_t modifiers;
  JavaString* signature;
  JavaObject* genericInfo;
  JavaObject* annotations;
  JavaObject* fieldAccessor;
  JavaObject* overrideFieldAccessor;
  JavaObject* root;
  JavaObject* securityCheckCache;
  JavaObject* securityCheckTargetClassCache;
  JavaObject* declaredAnnotations;

public:

  static void staticTracer(JavaObjectField* obj, word_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->clazz, closure);
    mvm::Collector::markAndTrace(obj, &obj->name, closure);
    mvm::Collector::markAndTrace(obj, &obj->type, closure);
    mvm::Collector::markAndTrace(obj, &obj->signature, closure);
    mvm::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->fieldAccessor, closure);
    mvm::Collector::markAndTrace(obj, &obj->overrideFieldAccessor, closure);
    mvm::Collector::markAndTrace(obj, &obj->root, closure);
    mvm::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->securityCheckTargetClassCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
  }

  static JavaField* getInternalField(JavaObjectField* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return &(cls->asClass()->virtualFields[self->slot]);
  }

  static UserClass* getClass(JavaObjectField* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return cls->asClass();
  }

};

class JavaObjectMethod : public JavaObject {
private:
  // AccessibleObject fields
  uint8 override;
  // Method fields
  JavaObjectClass* clazz;
  uint32 slot;
  JavaObject* name;
  JavaObject* returnType;
  JavaObject* parameterTypes;
  JavaObject* exceptionTypes;
  uint32_t modifiers;
  JavaString* Signature;
  JavaObject* genericInfo;
  JavaObject* annotations;
  JavaObject* parameterAnnotations;
  JavaObject* annotationDefault;
  JavaObject* methodAccessor;
  JavaObject* root;
  JavaObject* securityCheckCache;
  JavaObject* securityCheckTargetClassCache;
  JavaObject* declaredAnnotations;

public:

  static void staticTracer(JavaObjectMethod* obj, word_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->clazz, closure);
    mvm::Collector::markAndTrace(obj, &obj->name, closure);
    mvm::Collector::markAndTrace(obj, &obj->returnType, closure);
    mvm::Collector::markAndTrace(obj, &obj->parameterTypes, closure);
    mvm::Collector::markAndTrace(obj, &obj->exceptionTypes, closure);
    mvm::Collector::markAndTrace(obj, &obj->Signature, closure);
    mvm::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->parameterAnnotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotationDefault, closure);
    mvm::Collector::markAndTrace(obj, &obj->methodAccessor, closure);
    mvm::Collector::markAndTrace(obj, &obj->root, closure);
    mvm::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->securityCheckTargetClassCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
  }

  static JavaMethod* getInternalMethod(JavaObjectMethod* self);

  static UserClass* getClass(JavaObjectMethod* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return cls->asClass();
  }

};

class JavaObjectConstructor : public JavaObject {
private:
  // AccessibleObject fields
  uint8 override;
  // Constructor fields
  JavaObjectClass* clazz;
  uint32 slot;
  JavaObject* parameterTypes;
  JavaObject* exceptionTypes;
  uint32_t modifiers;
  JavaString* signature;
  JavaObject* genericInfo;
  JavaObject* annotations;
  JavaObject* parameterAnnotations;
  JavaObject* securityCheckCache;
  JavaObject* constructorAccessor;
  JavaObject* root;
  JavaObject* declaredAnnotations;

public:
  static void staticTracer(JavaObjectConstructor* obj, word_t closure) {
    mvm::Collector::markAndTrace(obj, &obj->clazz, closure);
    mvm::Collector::markAndTrace(obj, &obj->parameterTypes, closure);
    mvm::Collector::markAndTrace(obj, &obj->exceptionTypes, closure);
    mvm::Collector::markAndTrace(obj, &obj->signature, closure);
    mvm::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    mvm::Collector::markAndTrace(obj, &obj->annotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->parameterAnnotations, closure);
    mvm::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    mvm::Collector::markAndTrace(obj, &obj->constructorAccessor, closure);
    mvm::Collector::markAndTrace(obj, &obj->root, closure);
    mvm::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
  }

  static JavaMethod* getInternalMethod(JavaObjectConstructor* self);

  static UserClass* getClass(JavaObjectConstructor* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return cls->asClass();
  }

};

class JavaObjectThrowable : public JavaObject {
private:
  JavaObject* backtrace;
  JavaObject* detailedMessage;
  JavaObject* cause;
  JavaObject* stackTrace;

public:

  static void setDetailedMessage(JavaObjectThrowable* self, JavaObject* obj) {
    llvm_gcroot(self, 0);
    llvm_gcroot(obj, 0);
    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->detailedMessage), (gc*)obj);
  }

  static void fillInStackTrace(JavaObjectThrowable* self) {
    JavaObject* stackTrace = NULL;
    llvm_gcroot(self, 0);
    llvm_gcroot(stackTrace, 0);

    stackTrace = internalFillInStackTrace(self);
    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->backtrace), (gc*)stackTrace);

    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->cause), (gc*)self);

    self->stackTrace = NULL;
  }

  static int getStackTraceDepth(JavaObjectThrowable * self);
  static int getStackTraceBase(JavaObjectThrowable * self);
};

class JavaObjectReference : public JavaObject {
private:
  JavaObject* referent;
  JavaObject* queue;
  JavaObject* next;
  JavaObject* discovered;

public:
  static void init(JavaObjectReference* self, JavaObject* r, JavaObject* q) {
    llvm_gcroot(self, 0);
    llvm_gcroot(r, 0);
    llvm_gcroot(q, 0);
    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->referent), (gc*)r);
    mvm::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->queue), (gc*)q);
  }

  static JavaObject** getReferentPtr(JavaObjectReference* self) {
    llvm_gcroot(self, 0);
    return &(self->referent);
  }

  static void setReferent(JavaObjectReference* self, JavaObject* r) {
    llvm_gcroot(self, 0);
    llvm_gcroot(r, 0);
    // No write barrier: this is only called by the GC.
    self->referent = r;
  }
};

}

#endif
