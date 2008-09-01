//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"

#include "ClasspathConstructor.cpp.inc"
#include "ClasspathMethod.cpp.inc"
#include "ClasspathVMClass.cpp.inc"
#include "ClasspathVMClassLoader.cpp.inc"
#include "ClasspathVMField.cpp.inc"
#include "ClasspathVMObject.cpp.inc"
#include "ClasspathVMRuntime.cpp.inc"
#include "ClasspathVMStackWalker.cpp.inc"
#include "ClasspathVMSystem.cpp.inc"
#include "ClasspathVMSystemProperties.cpp.inc"
#include "ClasspathVMThread.cpp.inc"
#include "ClasspathVMThrowable.cpp.inc"

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

  UserCommonClass* cl = NativeUtil::resolvedImplClass(Cl, true);
  UserClass* methodCl = 0;
  if (cl->lookupMethodDontThrow(Jnjvm::clinitName, Jnjvm::clinitType, true,
                                false, methodCl))
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
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setByteNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jbyte val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setCharNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jchar val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setInt16Field((JavaObject*)obj, (uint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setShortNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jshort val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setInt16Field((JavaObject*)obj, (sint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setIntNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jint val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setInt32Field((JavaObject*)obj, (sint32)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setLongNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jlong val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setLongField((JavaObject*)obj, (sint64)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setFloatNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jfloat val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setFloatField((JavaObject*)obj, (float)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setDoubleNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jdouble val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setDoubleField((JavaObject*)obj, (double)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setObjectNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jobject val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  field->setObjectField((JavaObject*)obj, (JavaObject*)val);
}

JNIEXPORT jobject JNICALL Java_java_io_VMObjectInputStream_allocateObject(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass target, jclass constr, jobject cons) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(target, true);
  JavaObject* res = cl->doNew(vm);
  JavaMethod* meth = (JavaMethod*)(vm->upcalls->constructorSlot->getInt32Field((JavaObject*)cons));
  meth->invokeIntSpecial(vm, cl, res);
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
  UserCommonClass* base = NativeUtil::resolvedImplClass(arrayType, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = AssessorDesc::constructArrayName(loader, 0, 1, name);
  UserClassArray* array = loader->constructArray(arrayName);
  ArrayObject* res = ArrayObject::acons(arrayLength, array, &(vm->allocator));

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

JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapInt(
#ifdef NATIVE_JNI
    JNIEnv *env, 
#endif
    JavaObject* unsafe, JavaObject* obj, jlong offset, jint expect, jint update) {

  jint *ptr; 

  ptr = (jint *) (((uint8 *) obj) + offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapObject(
#ifdef NATIVE_JNI
    JNIEnv *env, 
#endif
    JavaObject* unsafe, JavaObject* obj, jlong offset, jobject expect, jobject update) {

  jobject *ptr; 

  ptr = (jobject *) (((uint8 *) obj) + offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

}
