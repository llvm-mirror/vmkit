//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"
#include "JavaClass.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"


using namespace jnjvm;

extern "C" {

// Define hasClassInitializer because of a buggy implementation in Classpath.
JNIEXPORT bool JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {
  
  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* cl = NativeUtil::resolvedImplClass(vm, Cl, true);
  UserClass* methodCl = 0;
  if (cl->lookupMethodDontThrow(Jnjvm::clinitName, Jnjvm::clinitType, true,
                                false, methodCl))
    return true;
  
  return false;
}


// Redefine some VMObjectStreamClass functions because of a slow implementation
// in Classpath.

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setBooleanNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jboolean val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setByteNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jbyte val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setCharNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jchar val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setInt16Field((JavaObject*)obj, (uint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setShortNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jshort val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setInt16Field((JavaObject*)obj, (sint16)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setIntNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jint val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setInt32Field((JavaObject*)obj, (sint32)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setLongNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jlong val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setLongField((JavaObject*)obj, (sint64)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setFloatNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jfloat val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setFloatField((JavaObject*)obj, (float)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setDoubleNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jdouble val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setDoubleField((JavaObject*)obj, (double)val);
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setObjectNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject Field, jobject obj, jobject val) {
  verifyNull(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  field->setObjectField((JavaObject*)obj, (JavaObject*)val);
}

JNIEXPORT jobject JNICALL Java_java_io_VMObjectInputStream_allocateObject(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass target, jclass constr, jobject cons) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(vm, target, true);
  JavaObject* res = cl->doNew(vm);
  JavaField* field = vm->upcalls->constructorSlot;
  JavaMethod* meth = (JavaMethod*)(field->getInt32Field((JavaObject*)cons));
  meth->invokeIntSpecial(vm, cl, res);
  return (jobject)res;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_VMArray_createObjectArray(
#ifdef NATIVE_JNI
JNIEnv * env,
jclass thisClass,
#endif
jclass arrayType, jint arrayLength) {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* base = NativeUtil::resolvedImplClass(vm, arrayType, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  UserClassArray* array = loader->constructArray(arrayName, base);
  return (jobject)array->doNew(arrayLength, vm);
}


JNIEXPORT 
bool JNICALL Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(
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
JavaObject* unsafe, JavaObject* obj, jlong offset, jobject expect,
jobject update) {

  jobject *ptr; 

  ptr = (jobject *) (((uint8 *) obj) + offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

}
