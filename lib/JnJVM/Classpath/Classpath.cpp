//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"


using namespace jnjvm;

extern "C" {

// Define hasClassInitializer because of a buggy implementation in Classpath.
JNIEXPORT bool JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {
  
  bool res = false;
  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)Cl, true);
  
  if (cl->isClass() && 
      cl->asClass()->lookupMethodDontThrow(vm->bootstrapLoader->clinitName,
                                           vm->bootstrapLoader->clinitType, 
                                           true, false, 0))
    res = true;

  END_NATIVE_EXCEPTION

  return res;
}


// Redefine some VMObjectStreamClass functions because of a slow implementation
// in Classpath.

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setBooleanNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jboolean val) {

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setInt8Field((JavaObject*)obj, (uint8)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setByteNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jbyte val) {

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setInt8Field((JavaObject*)obj, (uint8)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setCharNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jchar val) {

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setInt16Field((JavaObject*)obj, (uint16)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setShortNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jshort val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setInt16Field((JavaObject*)obj, (sint16)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setIntNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jint val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setInt32Field((JavaObject*)obj, (sint32)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setLongNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jlong val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setLongField((JavaObject*)obj, (sint64)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setFloatNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jfloat val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setFloatField((JavaObject*)obj, (float)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setDoubleNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jdouble val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setDoubleField((JavaObject*)obj, (double)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setObjectNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, jobject obj, jobject val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = Field->getInternalField();
  field->setObjectField((JavaObject*)obj, (JavaObject*)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT jobject JNICALL Java_java_io_VMObjectInputStream_allocateObject(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass target, jclass constr, JavaObjectConstructor* cons) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = 
    (UserClass*)UserCommonClass::resolvedImplClass(vm, (JavaObject*)target, true);
  JavaObject* obj = cl->doNew(vm);
  JavaMethod* meth = cons->getInternalMethod();
  meth->invokeIntSpecial(vm, cl, obj);
  res = (jobject)obj;

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_VMArray_createObjectArray(
#ifdef NATIVE_JNI
JNIEnv * env,
jclass thisClass,
#endif
jclass arrayType, jint arrayLength) {

  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* base = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)arrayType, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  UserClassArray* array = loader->constructArray(arrayName, base);
  res = (jobject)array->doNew(arrayLength, vm);

  END_NATIVE_EXCEPTION

  return res;
}


// Never throws.
JNIEXPORT 
bool JNICALL Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return false;
}

// Never throws.
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

// Never throws.
JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapInt(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObject* unsafe, JavaObject* obj, jlong offset, jint expect, jint update) {

  jint *ptr; 

  ptr = (jint *) (((uint8 *) obj) + offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

// Never throws.
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

// Never throws.
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObjectVolatile(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
jobject unsafe, jobject obj, jlong offset, jobject value) {

  jobject *ptr; 

  ptr = (jobject *) (((uint8 *) obj) + offset);

  *ptr = value;

}

}
