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

#include "ClasspathReflect.h"
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

// Never throws
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isArray(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject klass) {

  UserCommonClass* cl = ((JavaObjectClass*)klass)->getClass();

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

  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM(); 
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  UserCommonClass* cl = JCL->lookupClassFromJavaString((JavaString*)str, vm,
                                                        true, false);
  if (cl != 0) {
    if (clinit && cl->asClass()) {
      cl->asClass()->initialiseClass(vm);
    }
    res =(jclass)(cl->getClassDelegatee(vm));
  } else {
    vm->classNotFoundException((JavaString*)str);
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredConstructors(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl, 
jboolean publicOnly) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false);

  if (cl->isArray() || cl->isInterface() || cl->isPrimitive()) {
    result = (jobject)vm->upcalls->constructorArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = (Class*)cl;
    std::vector<JavaMethod*> res;
    realCl->getDeclaredConstructors(res, publicOnly);
    
    ArrayObject* ret = 
      (ArrayObject*)vm->upcalls->constructorArrayClass->doNew(res.size(), vm);
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
    result = (jobject)ret;
  }

  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredMethods(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl, 
jboolean publicOnly) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false);
  Classpath* upcalls = vm->upcalls;

  if (cl->isArray() || cl->isPrimitive()) {
    result = (jobject)upcalls->methodArrayClass->doNew(0, vm);
  } else {
    std::vector<JavaMethod*> res;
    UserClass* realCl = cl->asClass();
    realCl->getDeclaredMethods(res, publicOnly);
    
    ArrayObject* ret = 
      (ArrayObject*)upcalls->methodArrayClass->doNew(res.size(), vm);

    sint32 index = 0;
    for (std::vector<JavaMethod*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaMethod* meth = *i;
      // TODO: check parameter types
      UserClass* Meth = vm->upcalls->newMethod;
      JavaObject* tmp = Meth->doNew(vm);
      JavaString* str = vm->internalUTF8ToStr(meth->name);
      upcalls->initMethod->invokeIntSpecial(vm, Meth, tmp, Cl, str, meth);
      ret->elements[index] = tmp;
    }
    result = (jobject)ret;
  }

  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jint JNICALL Java_java_lang_VMClass_getModifiers(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl, 
jboolean ignore) {
  
  jint res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false);
  res = cl->getAccess();

  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jobject Cl) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  
  const UTF8* iname = cl->getName();
  const UTF8* res = iname->internalToJava(vm, 0, iname->size);

  result = (jobject)(vm->UTF8ToStr(res));

  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isPrimitive(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
 
  jboolean res = 0;
  BEGIN_NATIVE_EXCEPTION(0)
  
  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  
  res = cl->isPrimitive();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isInterface(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {

  jboolean res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false);

  res = cl->isInterface();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getComponentType(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  
  jclass res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();

  if (cl->isArray()) {
    UserCommonClass* bc = ((UserClassArray*)cl)->baseClass();
    res = (jclass)(bc->getClassDelegatee(vm));
  } else {
    res = 0;
  }

  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {

  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  res = (jobject)cl->classLoader->getJavaClassLoader();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isAssignableFrom(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl1, jclass Cl2) {
  
  jboolean res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)

  UserCommonClass* cl1 = ((JavaObjectClass*)Cl1)->getClass();
  UserCommonClass* cl2 = ((JavaObjectClass*)Cl2)->getClass();

  if (cl2->asClass()) cl2->asClass()->resolveClass();
  res = cl2->isAssignableFrom(cl1);

  END_NATIVE_EXCEPTION

  return res;

}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getSuperclass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {

  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  if (cl->isInterface()) res = 0;
  else {
    if (cl->asClass()) cl->asClass()->resolveClass();
    if (cl->getSuper()) res = (jobject)cl->getSuper()->getClassDelegatee(vm);
    else res = 0;
  }

  END_NATIVE_EXCEPTION
  
  return res;
}

JNIEXPORT bool JNICALL Java_java_lang_VMClass_isInstance(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jobject obj) {

  bool res = false;

  BEGIN_NATIVE_EXCEPTION(0)

  UserCommonClass* cl = ((JavaObjectClass*)Cl)->getClass();
  res = ((JavaObject*)obj)->instanceOf(cl);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredFields(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jboolean publicOnly) {

  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false)->asClass();

  if (!cl) {
    result = (jobject)vm->upcalls->fieldArrayClass->doNew(0, vm);
  } else {
    std::vector<JavaField*> res;
    cl->getDeclaredFields(res, publicOnly);
    
    ArrayObject* ret = 
      (ArrayObject*)vm->upcalls->fieldArrayClass->doNew(res.size(), vm);
    sint32 index = 0;
    for (std::vector<JavaField*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaField* field = *i;
      // TODO: check parameter types
      UserClass* Field = vm->upcalls->newField;
      JavaObject* tmp = Field->doNew(vm);
      JavaString* name = vm->internalUTF8ToStr(field->name);
      vm->upcalls->initField->invokeIntSpecial(vm, Field, tmp, Cl, name, field);
      ret->elements[index] = tmp;
    }
    result = (jobject)ret;
  }

  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getInterfaces(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {

  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false);
  ArrayObject* ret = 
    (ArrayObject*)vm->upcalls->classArrayClass->doNew(cl->nbInterfaces, vm);
  for (uint16 i = 0; i < cl->nbInterfaces; ++i) {
    UserClass* klass = cl->interfaces[i];
    ret->elements[i] = klass->getClassDelegatee(vm);
  }
  res = (jobject)ret;

  END_NATIVE_EXCEPTION

  return res;
}


JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getDeclaringClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false)->asClass();
  if (cl) {
    cl->resolveInnerOuterClasses();
    UserClass* outer = cl->getOuterClass();
    if (outer) {
      res = (jclass)outer->getClassDelegatee(vm);
    }
  }

  END_NATIVE_EXCEPTION

  return res;

}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredClasses(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, bool publicOnly) {


  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = NativeUtil::resolvedImplClass(vm, Cl, false)->asClass();
  if (cl) {
    cl->resolveInnerOuterClasses();
    UserClassArray* array = vm->upcalls->constructorArrayClass;
    ArrayObject* res = (ArrayObject*)array->doNew(cl->nbInnerClasses, vm);
    for (uint16 i = 0; i < cl->nbInnerClasses; ++i) {
      UserClass* klass = cl->innerClasses[i];
      res->elements[i] = klass->getClassDelegatee(vm); 
    }
    result = (jobject)res;
  }
  

  END_NATIVE_EXCEPTION

  return result;

}

// Only throws.
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
  
  jobjectArray res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassArray* array = vm->upcalls->constructorArrayAnnotation;
  res = (jobjectArray) array->doNew(0, vm);

  END_NATIVE_EXCEPTION

  return res;
}
}
