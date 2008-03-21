//===---- ClasspathVMClass.cpp - GNU classpath java/lang/VMClass ----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "llvm/Type.h"

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

  CommonClass* cl = 
    (CommonClass*)(((*Classpath::vmdataClass)((JavaObject*)klass)).PointerVal);

  return cl->isArray;
  
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
  CommonClass* cl = vm->lookupClassFromJavaString((JavaString*)str,
                                                  (JavaObject*)loader, true,
                                                  false, false);
  if (cl != 0) {
    if (clinit) {
      cl->initialiseClass();
    }
    return (jclass)(cl->getClassDelegatee());
  } else {
    vm->error(Jnjvm::ClassNotFoundException, "unable to load %s",
              ((JavaString*)str)->strToUTF8(vm)->printString());
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

  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  if (cl->isArray || isInterface(cl->access)) {
    return (jobject)ArrayObject::acons(0, Classpath::constructorArrayClass);
  } else {
    std::vector<JavaMethod*> meths = cl->virtualMethods;
    std::vector<JavaMethod*> res;
    for (std::vector<JavaMethod*>::iterator i = meths.begin(), e = meths.end();
          i != e; ++i) {
      if ((*i)->name == Jnjvm::initName && (!publicOnly || isPublic((*i)->access))) {
        res.push_back(*i);
      }
    }
    
    ArrayObject* ret = ArrayObject::acons(res.size(), Classpath::constructorArrayClass);
    sint32 index = 0;
    for (std::vector<JavaMethod*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaMethod* meth = *i;
      // TODO: check parameter types
      JavaObject* tmp = (*Classpath::newConstructor)();
      Classpath::initConstructor->invokeIntSpecial(tmp, Cl, meth);
      ret->setAt(index, tmp);
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
  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  if (cl->isArray) {
    return (jobject)ArrayObject::acons(0, Classpath::methodArrayClass);
  } else {
    std::vector<JavaMethod*> meths = cl->virtualMethods;
    std::vector<JavaMethod*> res;
    for (std::vector<JavaMethod*>::iterator i = meths.begin(), e = meths.end();
          i != e; ++i) {
      if ((*i)->name != Jnjvm::initName && (!publicOnly || isPublic((*i)->access))) {
        res.push_back(*i);
      }
    }
    meths = cl->staticMethods; 
    for (std::vector<JavaMethod*>::iterator i = meths.begin(), e = meths.end();
          i != e; ++i) {
      if ((*i)->name != Jnjvm::clinitName && (!publicOnly || isPublic((*i)->access))) {
        res.push_back(*i);
      }
    }
    
    ArrayObject* ret = ArrayObject::acons(res.size(), Classpath::methodArrayClass);
    sint32 index = 0;
    for (std::vector<JavaMethod*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaMethod* meth = *i;
      // TODO: check parameter types
      JavaObject* tmp = (*Classpath::newMethod)();
      Classpath::initMethod->invokeIntSpecial(tmp, Cl, vm->UTF8ToStr(meth->name), meth);
      ret->setAt(index, tmp);
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

  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);
  return cl->access;
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getName(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                         jclass clazz, 
#endif
                                                         jobject Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);
  
  AssessorDesc* ass = AssessorDesc::bogusClassToPrimitive(cl);
  const UTF8* res = 0;
  if (ass) {
    res = cl->name;
  } else {
    const UTF8* iname = cl->name;
    res = iname->internalToJava(vm, 0, iname->size);
  }
  return (jobject)(vm->UTF8ToStr(res));
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isPrimitive(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                              jclass clazz, 
#endif
                                                              jclass Cl) {
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);
  
  AssessorDesc* ass = AssessorDesc::bogusClassToPrimitive(cl);
  if (ass == 0) return false;
  else return true;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isInterface(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                              jclass clazz, 
#endif
                                                              jclass Cl) {
  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  return isInterface(cl->access);
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getComponentType(
#ifdef NATIVE_JNI
JNIEnv *env,
                                                                 jclass clazz, 
#endif
                                                                 jclass Cl) {
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);

  if (cl->isArray) {
    CommonClass* bc = ((ClassArray*)cl)->baseClass();
    return (jclass)(bc->getClassDelegatee());
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
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);
  return (jobject)cl->classLoader;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isAssignableFrom(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl1, jclass Cl2) {
  CommonClass* cl1 = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl1).PointerVal);
  CommonClass* cl2 = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl2).PointerVal);

  AssessorDesc* ass = AssessorDesc::bogusClassToPrimitive(cl1);
  if (ass) {
    return ass == AssessorDesc::bogusClassToPrimitive(cl2);
  } else {
    cl2->resolveClass(false);
    return cl2->isAssignableFrom(cl1);
  }

}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getSuperclass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);
  if (isInterface(cl->access))
    return 0;
  else {
    cl->resolveClass(false);
    if (cl->super) return (jobject)JavaThread::get()->isolate->getClassDelegatee(cl->super);
    else return 0;
  }
}

JNIEXPORT bool JNICALL Java_java_lang_VMClass_isInstance(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jobject obj) {
  CommonClass* cl = 
    (CommonClass*)((*Classpath::vmdataClass)((JavaObject*)Cl).PointerVal);
  return ((JavaObject*)obj)->instanceOf(cl);
}

JNIEXPORT jobject JNICALL Java_java_lang_VMClass_getDeclaredFields(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl, jboolean publicOnly) {
  Jnjvm* vm = JavaThread::get()->isolate;
  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);

  if (cl->isArray) {
    return (jobject)ArrayObject::acons(0, Classpath::fieldArrayClass);
  } else {
    std::vector<JavaField*> fields = cl->virtualFields;
    std::vector<JavaField*> res;
    for (std::vector<JavaField*>::iterator i = fields.begin(), e = fields.end();
          i != e; ++i) {
      if (!publicOnly || isPublic((*i)->access)) {
        res.push_back(*i);
      }
    }
    fields = cl->staticFields; 
    for (std::vector<JavaField*>::iterator i = fields.begin(), e = fields.end();
          i != e; ++i) {
      if (!publicOnly || isPublic((*i)->access)) {
        res.push_back(*i);
      }
    }
    
    ArrayObject* ret = ArrayObject::acons(res.size(), Classpath::fieldArrayClass);
    sint32 index = 0;
    for (std::vector<JavaField*>::iterator i = res.begin(), e = res.end();
          i != e; ++i, ++index) {
      JavaField* field = *i;
      // TODO: check parameter types
      JavaObject* tmp = (*Classpath::newField)();
      Classpath::initField->invokeIntSpecial(tmp, Cl, vm->UTF8ToStr(field->name), field);
      ret->setAt(index, tmp);
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
  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, false);
  std::vector<Class*> & interfaces = cl->interfaces;
  ArrayObject* ret = ArrayObject::acons(interfaces.size(), Classpath::classArrayClass);
  sint32 index = 0;
  for (std::vector<Class*>::iterator i = interfaces.begin(), e = interfaces.end();
        i != e; ++i, ++index) {
    Class* klass = *i; 
    ret->setAt(index, vm->getClassDelegatee(klass));
  }
  return (jobject)ret;
}

static void resolveInnerOuterClasses(Class* cl) {
  Attribut* attribut = Attribut::lookup(&cl->attributs,
                                        Attribut::innerClassesAttribut);
  if (attribut != 0) {
    Reader* reader = attribut->toReader(cl->bytes, attribut);

    uint16 nbi = reader->readU2();
    for (uint16 i = 0; i < nbi; ++i) {
      uint16 inner = reader->readU2();
      uint16 outer = reader->readU2();
      //uint16 innerName = 
      reader->readU2();
      uint16 accessFlags = reader->readU2();
      Class* clInner = (Class*)cl->ctpInfo->loadClass(inner);
      Class* clOuter = (Class*)cl->ctpInfo->loadClass(outer);

      if (clInner == cl) {
        cl->outerClass = clOuter;
      } else if (clOuter == cl) {
        clInner->innerAccess = accessFlags;
        cl->innerClasses.push_back(clInner);
      }
    }
  }
  cl->innerOuterResolved = true;
}


JNIEXPORT jclass JNICALL Java_java_lang_VMClass_getDeclaringClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jclass Cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  Class* cl = (Class*)NativeUtil::resolvedImplClass(Cl, false);
  if (!(cl->innerOuterResolved))
    resolveInnerOuterClasses(cl);
  if (cl->outerClass) {
    return (jclass)vm->getClassDelegatee(cl->outerClass);
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
  Class* cl = (Class*)NativeUtil::resolvedImplClass(Cl, false);
  if (!(cl->innerOuterResolved))
    resolveInnerOuterClasses(cl);
  ArrayObject* res = ArrayObject::acons(cl->innerClasses.size(), Classpath::constructorArrayClass);
  uint32 index = 0;
  for (std::vector<Class*>::iterator i = cl->innerClasses.begin(), 
       e = cl->innerClasses.end(); i!= e; i++) {
    res->elements[index++] = vm->getClassDelegatee(*i); 
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

}
