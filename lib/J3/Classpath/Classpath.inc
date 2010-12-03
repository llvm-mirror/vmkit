//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                            The VMKit project
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


using namespace j3;

extern "C" {

// Define hasClassInitializer because of a buggy implementation in Classpath.
JNIEXPORT bool JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* Cl) {
  
  llvm_gcroot(Cl, 0);
  bool res = false;
  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, true);
  
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
JavaObjectField* Field, JavaObject* obj, jboolean val) {

  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceInt8Field(obj, (uint8)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setByteNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jbyte val) {

  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceInt8Field(obj, (uint8)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setCharNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jchar val) {

  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceInt16Field((JavaObject*)obj, (uint16)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setShortNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jshort val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceInt16Field(obj, (sint16)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setIntNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jint val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceInt32Field(obj, (sint32)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setLongNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jlong val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceLongField(obj, (sint64)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setFloatNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jfloat val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceFloatField(obj, (float)val);
  
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setDoubleNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, jdouble val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceDoubleField(obj, (double)val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_io_VMObjectStreamClass_setObjectNative(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObjectField* Field, JavaObject* obj, JavaObject* val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(val, 0);
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(obj);
  JavaField* field = JavaObjectField::getInternalField(Field);
  field->setInstanceObjectField(obj, val);

  END_NATIVE_EXCEPTION
}

JNIEXPORT JavaObject* JNICALL Java_java_io_VMObjectInputStream_allocateObject(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* target, JavaObject* constr, JavaObjectConstructor* cons) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(target, 0);
  llvm_gcroot(constr, 0);
  llvm_gcroot(cons, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = 
    (UserClass*)UserCommonClass::resolvedImplClass(vm, target, true);
  res = cl->doNew(vm);
  JavaMethod* meth = JavaObjectConstructor::getInternalMethod(cons);
  meth->invokeIntSpecial(vm, cl, res);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMArray_createObjectArray(
#ifdef NATIVE_JNI
JNIEnv * env,
jclass thisClass,
#endif
JavaObject* arrayType, jint arrayLength) {

  JavaObject* res = 0;
  llvm_gcroot(arrayType, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* base = 
    UserCommonClass::resolvedImplClass(vm, arrayType, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  UserClassArray* array = loader->constructArray(arrayName, base);
  res = array->doNew(arrayLength, vm);

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

  llvm_gcroot(unsafe, 0);
  llvm_gcroot(obj, 0);
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

  llvm_gcroot(unsafe, 0);
  llvm_gcroot(obj, 0);
  jint *ptr; 

  ptr = (jint *) (((uint8 *) obj) + offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

// Never throws.
JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapObject(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObject* unsafe, JavaObject* obj, jlong offset, JavaObject* expect,
JavaObject* update) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(expect, 0);
  llvm_gcroot(update, 0);

  JavaObject** ptr = (JavaObject**) (((uint8 *) obj) + offset);

  return mvm::Collector::objectReferenceTryCASBarrier((gc*)obj, (gc**)ptr, (gc*)expect, (gc*)update);
}

// Never throws.
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObjectVolatile(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObject* unsafe, JavaObject* obj, jlong offset, JavaObject* value) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(value, 0);

  JavaObject** ptr = (JavaObject**) (((uint8 *) obj) + offset);
  mvm::Collector::objectReferenceWriteBarrier((gc*)obj, (gc**)ptr, (gc*)value);
}

}
