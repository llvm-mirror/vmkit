//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"

#include "ClasspathConstructor.cpp"
#include "ClasspathMethod.cpp"
#include "ClasspathVMClass.cpp"
#include "ClasspathVMClassLoader.cpp"
#include "ClasspathVMField.cpp"
#include "ClasspathVMObject.cpp"
#include "ClasspathVMRuntime.cpp"
#include "ClasspathVMStackWalker.cpp"
#include "ClasspathVMSystem.cpp"
#include "ClasspathVMSystemProperties.cpp"
#include "ClasspathVMThread.cpp"
#include "ClasspathVMThrowable.cpp"

#include "JavaClass.h"
#include "Jnjvm.h"
#include "NativeUtil.h"


// Called by JnJVM to ensure the compiler will link the classpath methods
extern "C" int ClasspathBoot(int argc, char** argv, char** env) {
  return 1;
}

using namespace jnjvm;

extern "C" {
JNIEXPORT bool JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {

  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, true);
  if (cl->lookupMethodDontThrow(Jnjvm::clinitName, Jnjvm::clinitType, true, false))
    return true;
  else
    return false;
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setBooleanNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jboolean val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setByteNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jbyte val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setCharNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jchar val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualInt16Field((JavaObject*)obj, (uint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setShortNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jshort val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualInt16Field((JavaObject*)obj, (sint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setIntNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jint val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setLongNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jlong val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualLongField((JavaObject*)obj, (sint64)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setFloatNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jfloat val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualFloatField((JavaObject*)obj, (float)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setDoubleNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jdouble val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualDoubleField((JavaObject*)obj, (double)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setObjectNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jobject val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  field->setVirtualObjectField((JavaObject*)obj, (JavaObject*)val);
}

JNIEXPORT jobject JNICALL Java_java_io_VMObjectInputStream_allocateObject(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass target, jclass constr, jobject cons) {
  Jnjvm* vm = JavaThread::get()->isolate;
  Class* cl = (Class*)NativeUtil::resolvedImplClass(target, true);
  JavaObject* res = cl->doNew(vm);
  JavaMethod* meth = (JavaMethod*)(Classpath::constructorSlot->getVirtualInt32Field((JavaObject*)cons));
  meth->invokeIntSpecial(vm, res);
  return (jobject)res;
}

JNIEXPORT jobject JNICALL
Java_java_lang_reflect_VMArray_createObjectArray
  (
#ifdef NATIVE_JNI
   JNIEnv * env,
   jclass thisClass __attribute__ ((__unused__)),
#endif
   jclass arrayType, jint arrayLength)
{
  Jnjvm* vm = JavaThread::get()->isolate;
  CommonClass* base = NativeUtil::resolvedImplClass(arrayType, true);
  JavaObject* loader = base->classLoader;
  const UTF8* name = base->name;
  const UTF8* arrayName = AssessorDesc::constructArrayName(vm, 0, 1, name);
  ClassArray* array = vm->constructArray(arrayName, loader);
  ArrayObject* res = ArrayObject::acons(arrayLength, array, vm);

  return (jobject) res;
}


JNIEXPORT bool JNICALL Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return false;
}

JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(
#ifdef NATIVE_JNI
    JNIEnv *env, 
#endif
    JavaObject* unsafe, JavaObject* obj, jlong offset, jlong expect, jlong update) {

  jlong *ptr; 
  jlong  value;

  ptr = (jlong *) (((uint8 *) obj) + offset);

  value = *ptr;

  if (value == expect) {
    *ptr = update;
    return true;
  } else {
    return false;
  }

}

}
