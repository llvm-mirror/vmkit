//===---- ClasspathVMClass.cpp - GNU classpath java/lang/VMClass ----------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;

extern "C" {

// Never throws
JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isArray(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectClass* klass) {

  llvm_gcroot(klass, 0);

  UserCommonClass* cl = JavaObjectClass::getClass(klass);

  return cl->isArray();
  
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_forName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaString* str, 
jboolean clinit, 
JavaObject* loader) {

  JavaObject* res = 0;
  llvm_gcroot(loader, 0);
  llvm_gcroot(str, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(str);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);
  UserCommonClass* cl = JCL->loadClassFromJavaString(str, true, false);

  if (cl != NULL && !cl->isPrimitive()) {
    if (clinit && cl->asClass()) {
      cl->asClass()->initialiseClass(vm);
    }
    res = cl->getClassDelegatee(vm);
  } else {
    vm->classNotFoundException(str);
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getDeclaredConstructors(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* Cl, 
jboolean publicOnly) {
  JavaObjectClass* jcl = 0;
  ArrayObject* ret = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  ret = JavaObjectClass::getDeclaredConstructors(jcl = (JavaObjectClass*)Cl, publicOnly);

  END_NATIVE_EXCEPTION

  return ret;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getDeclaredMethods(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* Cl, 
jboolean publicOnly) {

  JavaObjectClass* jcl = 0;
  ArrayObject* ret = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  ret = JavaObjectClass::getDeclaredMethods(jcl = (JavaObjectClass*)Cl, publicOnly);

  END_NATIVE_EXCEPTION

  return ret;
}

JNIEXPORT jint JNICALL Java_java_lang_VMClass_getModifiers(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* Cl, 
jboolean ignore) {
  
  JavaObjectClass* jcl = 0;
  jint res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  res = JavaObjectClass::getModifiers(jcl = (JavaObjectClass*)Cl);

  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getName(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {

  JavaObject* result = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(result, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  
  const UTF8* iname = cl->getName();
  result = JavaString::internalToJava(iname, vm);

  END_NATIVE_EXCEPTION

  return result;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isPrimitive(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
 
  jboolean res = 0;
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  
  res = cl->isPrimitive();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isInterface(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {

  jboolean res = 0;
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  res = cl->isInterface();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getComponentType(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Cl, 0);

  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (cl->isArray()) {
    UserCommonClass* bc = cl->asArrayClass()->baseClass();
    res = bc->getClassDelegatee(vm);
  } else {
    res = 0;
  }

  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getClassLoader(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {

  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = cl->classLoader->getJavaClassLoader();

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_VMClass_isAssignableFrom(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl1, JavaObject* Cl2) {
  
  jboolean res = 0;
  llvm_gcroot(Cl1, 0);
  llvm_gcroot(Cl2, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  if (!Cl2) vm->nullPointerException();

  UserCommonClass* cl1 = UserCommonClass::resolvedImplClass(vm, Cl1, false);
  UserCommonClass* cl2 = UserCommonClass::resolvedImplClass(vm, Cl2, false);

  res = cl2->isSubclassOf(cl1);

  END_NATIVE_EXCEPTION

  return res;

}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getSuperclass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {

  JavaObject* res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  if (cl->isInterface()) res = 0;
  else {
    if (cl->getSuper()) res = cl->getSuper()->getClassDelegatee(vm);
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
JavaObject* Cl, JavaObject* obj) {

  bool res = false;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = JavaObject::instanceOf(obj, cl);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getDeclaredFields(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl, jboolean publicOnly) {
  JavaObjectClass* jcl = 0;
  ArrayObject* ret = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  ret = JavaObjectClass::getDeclaredFields(jcl = (JavaObjectClass*)Cl, publicOnly);

  END_NATIVE_EXCEPTION

  return ret;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getInterfaces(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
  JavaObjectClass* jcl = 0;
  ArrayObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Cl, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  res = JavaObjectClass::getInterfaces(jcl = (JavaObjectClass*)Cl);

  END_NATIVE_EXCEPTION

  return res;
}


JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getDeclaringClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
  JavaObjectClass* jcl = 0;
  JavaObject* res = 0;

  llvm_gcroot(res, 0);
  llvm_gcroot(Cl, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  res = JavaObjectClass::getDeclaringClass(jcl = (JavaObjectClass*)Cl);

  END_NATIVE_EXCEPTION

  return res;

}

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMClass_getDeclaredClasses(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl, bool publicOnly) {
  JavaObjectClass* jcl = 0;
  ArrayObject* result = 0;
  llvm_gcroot(result, 0);
  llvm_gcroot(Cl, 0);
  llvm_gcroot(jcl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  result = JavaObjectClass::getDeclaredClasses(jcl = (JavaObjectClass*)Cl, publicOnly);

  END_NATIVE_EXCEPTION

  return result;

}

// Only throws.
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* throwable) {
  
  llvm_gcroot(throwable, 0);

  assert(throwable && "Using internal VM throw exception without exception");
  JavaThread::get()->throwException(throwable);
}

JNIEXPORT ArrayObject* Java_java_lang_VMClass_getDeclaredAnnotations(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
  // TODO implement me
	//fprintf(stderr, "Java_java_lang_VMClass_getDeclaredAnnotations not implemented\n");
  //abort();

  ArrayObject* arrayRes = 0;
  JavaObject* res = 0;
  JavaObject* annon = 0;
  JavaObject* newHashMap = 0;
  JavaObject* annotationClass = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(annon, 0);
  llvm_gcroot(newHashMap, 0);
  llvm_gcroot(Cl, 0);
  llvm_gcroot(annotationClass, 0);
  llvm_gcroot(arrayRes, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  Classpath* upcalls = vm->upcalls;

  UserClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false)->asClass();
  //JavaField* field = JavaObjectVMField::getInternalField(VMField);
  JavaAttribute* annotationsAtt =
    cl->lookupAttribute(JavaAttribute::annotationsAttribute);
  //dprintf("JavaClass : %s\n", UTF8Buffer(field->name).cString());
  
	JnjvmClassLoader* loader = cl->classLoader;

  if (annotationsAtt) {
    Reader reader(annotationsAtt, cl->bytes);
    AnnotationReader AR(reader, cl);
    uint16 numAnnotations = reader.readU2();
    
    UserClassArray* array = upcalls->annotationArrayClass;
    arrayRes = (ArrayObject *)(res = array->doNew(numAnnotations, vm));
    for (uint16 i = 0; i < numAnnotations; ++i) {
			uint16 typeIndex = reader.readU2();
			const UTF8* annoType = cl->ctpInfo->UTF8At(typeIndex);
			newHashMap = AR.createAnnotationMapValues();
			
			annoType = annoType->extract(loader->hashUTF8, 1,annoType->size-1);
			dprintf("Annotation type : %s\n", UTF8Buffer(annoType).cString());
			UserClass* AnnonClass = 0;
			UserCommonClass* commClass = loader->lookupClass(annoType);
			
			if (commClass) 
				AnnonClass = commClass -> asClass();
			else
				AnnonClass = loader->loadName(annoType, true, true, NULL);
				
			assert(AnnonClass && "Panic, imposible conditions");
	
			dprintf("Loaded class : %s \n", UTF8Buffer(AnnonClass->name).cString());
			annotationClass = AnnonClass->getClassDelegatee(vm);
			
			annon = upcalls->createAnnotation->invokeJavaObjectStatic(vm, upcalls->newAnnotationHandler, &annotationClass, &newHashMap);
			ArrayObject::setElement(arrayRes, annon, i);
		} // end for
		
  }

  END_NATIVE_EXCEPTION
  return (ArrayObject*)res;
}

JNIEXPORT jboolean Java_java_lang_VMClass_isAnonymousClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* Cl) {
 
  jboolean res = false;
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false)->asClass();

  if (cl) res = cl->isAnonymous;

  END_NATIVE_EXCEPTION

  return res;
}

}
