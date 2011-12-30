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

#include "VmkitGC.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaString.h"
#include "Jnjvm.h"
#include "JavaUpcalls.h"

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
  JavaObject* declaredPublicMethods;
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
    vmkit::Collector::objectReferenceWriteBarrier(
        (gc*)cl, (gc**)&(cl->pd), (gc*)pd);
  }

  static JavaObject* getProtectionDomain(JavaObjectClass* cl) {
    llvm_gcroot(cl, 0);
    return cl->pd;
  }

  static void staticTracer(JavaObjectClass* obj, word_t closure) {
    vmkit::Collector::markAndTrace(obj, &obj->cachedConstructor, closure);
    vmkit::Collector::markAndTrace(obj, &obj->newInstanceCallerCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->name, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredFields, closure);
    vmkit::Collector::markAndTrace(obj, &obj->publicFields, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredMethods, closure);
    vmkit::Collector::markAndTrace(obj, &obj->publicMethods, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredConstructors, closure);
    vmkit::Collector::markAndTrace(obj, &obj->publicConstructors, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredPublicFields, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredPublicMethods, closure);
    vmkit::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    vmkit::Collector::markAndTrace(obj, &obj->enumConstants, closure);
    vmkit::Collector::markAndTrace(obj, &obj->enumConstantDictionary, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotationType, closure);

    UserCommonClass * cl = getClass(obj);
    if (cl) {
      JavaObject** Obj = cl->classLoader->getJavaClassLoaderPtr();
      if (*Obj) vmkit::Collector::markAndTraceRoot(Obj, closure);
    }
  }

  static ArrayObject* getDeclaredClasses(JavaObjectClass* Cl, bool publicOnly);
  static ArrayObject* getDeclaredConstructors(JavaObjectClass* Cl, bool publicOnly);
  static ArrayObject* getDeclaredFields(JavaObjectClass* Cl, bool publicOnly);
  static ArrayObject* getDeclaredMethods(JavaObjectClass* Cl, bool publicOnly);
  static ArrayObject* getInterfaces(JavaObjectClass* Cl);
  static JavaObject* getDeclaringClass(JavaObjectClass* Cl);
  static int getModifiers(JavaObjectClass* Cl);
  static JavaString** getSignature(Class* cl);
  static ArraySInt8* getAnnotations(Class* cl);
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
    vmkit::Collector::markAndTrace(obj, &obj->clazz, closure);
    vmkit::Collector::markAndTrace(obj, &obj->name, closure);
    vmkit::Collector::markAndTrace(obj, &obj->type, closure);
    vmkit::Collector::markAndTrace(obj, &obj->signature, closure);
    vmkit::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->fieldAccessor, closure);
    vmkit::Collector::markAndTrace(obj, &obj->overrideFieldAccessor, closure);
    vmkit::Collector::markAndTrace(obj, &obj->root, closure);
    vmkit::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->securityCheckTargetClassCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
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

  static JavaObjectField* createFromInternalField(JavaField* field, int i);
  static JavaString** getSignature(JavaField* field);
  static ArraySInt8* getAnnotations(JavaField* field);
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
    vmkit::Collector::markAndTrace(obj, &obj->clazz, closure);
    vmkit::Collector::markAndTrace(obj, &obj->name, closure);
    vmkit::Collector::markAndTrace(obj, &obj->returnType, closure);
    vmkit::Collector::markAndTrace(obj, &obj->parameterTypes, closure);
    vmkit::Collector::markAndTrace(obj, &obj->exceptionTypes, closure);
    vmkit::Collector::markAndTrace(obj, &obj->Signature, closure);
    vmkit::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->parameterAnnotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotationDefault, closure);
    vmkit::Collector::markAndTrace(obj, &obj->methodAccessor, closure);
    vmkit::Collector::markAndTrace(obj, &obj->root, closure);
    vmkit::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->securityCheckTargetClassCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
  }

  static JavaMethod* getInternalMethod(JavaObjectMethod* self);

  static UserClass* getClass(JavaObjectMethod* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return cls->asClass();
  }

  static JavaObjectMethod* createFromInternalMethod(JavaMethod* meth, int i);
  static JavaString** getSignature(JavaMethod* meth);
  static ArraySInt8* getAnnotations(JavaMethod* meth);
  static ArraySInt8* getParamAnnotations(JavaMethod* meth);
  static ArraySInt8* getAnnotationDefault(JavaMethod* meth);
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
    vmkit::Collector::markAndTrace(obj, &obj->clazz, closure);
    vmkit::Collector::markAndTrace(obj, &obj->parameterTypes, closure);
    vmkit::Collector::markAndTrace(obj, &obj->exceptionTypes, closure);
    vmkit::Collector::markAndTrace(obj, &obj->signature, closure);
    vmkit::Collector::markAndTrace(obj, &obj->genericInfo, closure);
    vmkit::Collector::markAndTrace(obj, &obj->annotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->parameterAnnotations, closure);
    vmkit::Collector::markAndTrace(obj, &obj->securityCheckCache, closure);
    vmkit::Collector::markAndTrace(obj, &obj->constructorAccessor, closure);
    vmkit::Collector::markAndTrace(obj, &obj->root, closure);
    vmkit::Collector::markAndTrace(obj, &obj->declaredAnnotations, closure);
  }

  static JavaMethod* getInternalMethod(JavaObjectConstructor* self);

  static UserClass* getClass(JavaObjectConstructor* self) {
    llvm_gcroot(self, 0);
    UserCommonClass* cls = JavaObjectClass::getClass(self->clazz);
    return cls->asClass();
  }

  static JavaObjectConstructor* createFromInternalConstructor(JavaMethod* cons, int i);
  static JavaString** getSignature(JavaMethod* cons);
  static ArraySInt8* getAnnotations(JavaMethod* cons);
  static ArraySInt8* getParamAnnotations(JavaMethod* cons);
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
    vmkit::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->detailedMessage), (gc*)obj);
  }

  static void fillInStackTrace(JavaObjectThrowable* self) {
    JavaObject* stackTrace = NULL;
    llvm_gcroot(self, 0);
    llvm_gcroot(stackTrace, 0);

    stackTrace = internalFillInStackTrace(self);
    vmkit::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->backtrace), (gc*)stackTrace);

    vmkit::Collector::objectReferenceWriteBarrier(
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
    vmkit::Collector::objectReferenceWriteBarrier(
        (gc*)self, (gc**)&(self->referent), (gc*)r);
    if (!q) {
      JavaField * field = JavaThread::get()->getJVM()->upcalls->NullRefQueue;
      q = field->getStaticObjectField();
    }
    assert(q);
    vmkit::Collector::objectReferenceWriteBarrier(
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
