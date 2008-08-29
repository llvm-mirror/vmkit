//===---- ClasspathVMClass.cpp - GNU classpath java/lang/VMClass ----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "types.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"
#include "Reader.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isArray(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject klass) {

  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)klass);

  return cl->isArray();
  
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClass_forName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif

                                                        jobject str, 
                                                        jboolean clinit, 
                                                        jobject loader) {

  Jnjvm* vm = JavaThread::get()->isolate; 
  JnjvmClassLoader* JCL = JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  UserCommonClass* cl = JCL->lookupClassFromJavaString((JavaString*)str,
                                                   true, false);
  if (cl != 0) {
    if (clinit) {
      cl->initialiseClass(vm);
    }
    return (jclass)(cl->getClassDelegatee(vm));
  } else {
    vm->classNotFoundException((JavaString*)str);
    return 0;
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredConstructors(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                                         jclass clazz,
#endif

                                                                         jclass Cl, 
                                                                         jboolean publicOnly) {

  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);
  Jnjvm* vm = JavaThread::get()->isolate;

  if (cl->isArray() || cl->isInterface()) {
    return (jobject)ArrayObject::acons(0, vm->upcalls->constructorArrayClass, &(vm->allocator));
  } else {
    std::vector<JavaMethod*> res;
    cl->getDeclaredConstructors(res, publicOnly);
    
    ArrayObject* ret = ArrayObject::acons(res.size(), vm->upcalls->constructorArrayClass, &(vm->allocator));
    sint32 index = 0;
    for (std::vector<JavaMethod*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaMethod* meth = *i;
      // TODO: check parameter types
      UserClass* Cons = vm->upcalls->newConstructor;
      JavaObject* tmp = Cons->doNew(vm);
      vm->upcalls->initConstructor->invokeIntSpecial(vm, Cons, tmp, Cl, meth);
      ret->elements[index] = tmp;
    }
    return (jobject)ret;
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredMethods(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                                    jclass clazz,
#endif

                                                                    jclass Cl, 
                                                                    jboolean publicOnly) {

  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  if (cl->isArray()) {
    return (jobject)ArrayObject::acons(0, vm->upcalls->methodArrayClass, &(vm->allocator));
  } else {
    std::vector<JavaMethod*> res;
    cl->getDeclaredMethods(res, publicOnly);
    
    ArrayObject* ret = ArrayObject::acons(res.size(), vm->upcalls->methodArrayClass, &(vm->allocator));
    sint32 index = 0;
    for (std::vector<JavaMethod*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaMethod* meth = *i;
      // TODO: check parameter types
      UserClass* Meth = vm->upcalls->newMethod;
      JavaObject* tmp = Meth->doNew(vm);
      vm->upcalls->initMethod->invokeIntSpecial(vm, Meth, tmp, Cl,
                                              vm->UTF8ToStr(meth->name), meth);
      ret->elements[index] = tmp;
    }
    return (jobject)ret;
  }
}

JNIEXPORT jint JNICALL Java_java_lang_VMClass_getModifiers(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                           jclass clazz,
#endif
                                                           jclass Cl, 
                                                           jboolean ignore) {

  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);
  return cl->getAccess();
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getName(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                         jclass clazz, 
#endif
                                                         jobject Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);
  
  const UTF8* iname = cl->getName();
  const UTF8* res = iname->internalToJava(cl->classLoader->hashUTF8, 0, iname->size);

  return (jobject)(vm->UTF8ToStr(res));
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isPrimitive(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                              jclass clazz, 
#endif
                                                              jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);
  
  return cl->isPrimitive();
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isInterface(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                              jclass clazz, 
#endif
                                                              jclass Cl) {
  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  return cl->isInterface();
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getComponentType(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                                 jclass clazz, 
#endif
                                                                 jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);

  if (cl->isArray()) {
    UserCommonClass* bc = ((UserClassArray*)cl)->baseClass();
    return (jclass)(bc->getClassDelegatee(vm));
  } else {
    return 0;
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);
  return (jobject)cl->classLoader->getJavaClassLoader();
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isAssignableFrom(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl1, jclass Cl2) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl1 = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl1);
  UserCommonClass* cl2 = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl2);

  cl2->resolveClass();
  return cl2->isAssignableFrom(cl1);

}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getSuperclass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);
  if (cl->isInterface())
    return 0;
  else {
    cl->resolveClass();
    if (cl->getSuper()) return (jobject)cl->getSuper()->getClassDelegatee(vm);
    else return 0;
  }
}

JNIEXPORT bool JNICALL Java_java_lang_VMClass_isInstance(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField((JavaObject*)Cl);
  return ((JavaObject*)obj)->instanceOf(cl);
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredFields(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jboolean publicOnly) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  if (cl->isArray()) {
    return (jobject)ArrayObject::acons(0, vm->upcalls->fieldArrayClass, &(vm->allocator));
  } else {
    std::vector<JavaField*> res;
    cl->getDeclaredFields(res, publicOnly);
    
    ArrayObject* ret = ArrayObject::acons(res.size(),
                                          vm->upcalls->fieldArrayClass, &(vm->allocator));
    sint32 index = 0;
    for (std::vector<JavaField*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaField* field = *i;
      // TODO: check parameter types
      UserClass* Field = vm->upcalls->newField;
      JavaObject* tmp = Field->doNew(vm);
      vm->upcalls->initField->invokeIntSpecial(vm, Field, tmp, Cl,
                                             vm->UTF8ToStr(field->name), field);
      ret->elements[index] = tmp;
    }
    return (jobject)ret;
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getInterfaces(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);
  std::vector<UserClass*> * interfaces = cl->getInterfaces();
  ArrayObject* ret = ArrayObject::acons(interfaces->size(), vm->upcalls->classArrayClass, &(vm->allocator));
  sint32 index = 0;
  for (std::vector<UserClass*>::iterator i = interfaces->begin(), e = interfaces->end();
        i != e; ++i, ++index) {
    UserClass* klass = *i; 
    ret->elements[index] = klass->getClassDelegatee(vm);
  }
  return (jobject)ret;
}


JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getDeclaringClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(Cl, false);
  cl->resolveInnerOuterClasses();
  UserClass* outer = cl->getOuterClass();
  if (outer) {
    return (jclass)outer->getClassDelegatee(vm);
  } else {
    return 0;
  }
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredClasses(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, bool publicOnly) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(Cl, false);
  cl->resolveInnerOuterClasses();
  std::vector<UserClass*>* innerClasses = cl->getInnerClasses();
  ArrayObject* res = ArrayObject::acons(innerClasses->size(), vm->upcalls->constructorArrayClass, &(vm->allocator));
  uint32 index = 0;
  for (std::vector<UserClass*>::iterator i = innerClasses->begin(), 
       e = innerClasses->end(); i!= e; i++) {
    res->elements[index++] = (*i)->getClassDelegatee(vm); 
  }

  return (jobject)res;
}


JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jobject throwable) {
  JavaThread::throwException((JavaObject*)throwable);
}

JNIEXPORT jobjectArray Java_java_lang_VMClass_getDeclaredAnnotations(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  // TODO implement me
  Jnjvm* vm = JavaThread::get()->isolate;
  ArrayObject* res = ArrayObject::acons(0, vm->upcalls->constructorArrayAnnotation, &(vm->allocator));
  return (jobjectArray)res;
}
}
