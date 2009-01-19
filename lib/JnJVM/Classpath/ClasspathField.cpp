//===- ClasspathVMField.cpp - GNU classpath java/lang/reflect/Field -------===//
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
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace jnjvm;

extern "C" {


static UserClass* internalGetClass(Jnjvm* vm, jobject Field) {
  JavaField* slot = vm->upcalls->fieldClass;
  JavaObject* Cl = (JavaObject*)slot->getObjectField((JavaObject*)Field);
  UserClass* cl = (UserClass*)UserCommonClass::resolvedImplClass(vm, Cl, false);
  return cl;
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  jint res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  res = field->access;

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  JnjvmClassLoader* loader = cl->classLoader;
  UserCommonClass* fieldCl = field->getSignature()->assocClass(loader);
  res = (jclass)fieldCl->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  
  jint res = 0;
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  const Typedef* type = field->getSignature();
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;

    if (prim->isInt())
      res = (sint32)field->getInt32Field(Obj);
    else if (prim->isChar())
      res = (uint32)field->getInt16Field(Obj);
    else if (prim->isByte())
      res = (sint32)field->getInt8Field(Obj);
    else if (prim->isShort())
      res = (sint32)field->getInt16Field(Obj);
    else
      vm->illegalArgumentException("");
  } else {
      vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION
  
  return res;
  
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {

  jlong res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    
    if (prim->isInt())
      res = (sint64)field->getInt32Field(Obj);
    else if (prim->isChar())
      res = (uint64)field->getInt16Field(Obj);
    else if (prim->isByte())
      res = (sint64)field->getInt8Field(Obj);
    else if (prim->isShort())
      res = (sint64)field->getInt16Field(Obj);
    else if (prim->isLong())
      res = (sint64)field->getLongField(Obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {

  jboolean res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())  
      res = (uint8)field->getInt8Field(Obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION

  return res;
  
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  
  jfloat res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (jfloat)field->getInt8Field(Obj);
    else if (prim->isInt())
      res = (jfloat)field->getInt32Field((JavaObject*)obj);
    else if (prim->isShort())
      res = (jfloat)field->getInt16Field((JavaObject*)obj);
    else if (prim->isLong())
      res = (jfloat)field->getLongField((JavaObject*)obj);
    else if (prim->isChar())
      // Cast to uint32 because char is unsigned.
      res = (jfloat)(uint32)field->getInt16Field((JavaObject*)obj);
    else if (prim->isFloat())
      res = (jfloat)field->getFloatField((JavaObject*)obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {

  jbyte res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (sint8)field->getInt8Field(Obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION
  
  return res;
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  
  jchar res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      res = (uint16)field->getInt16Field((JavaObject*)obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
  
  return res;
  
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {


  jshort res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      res = (sint16)field->getInt16Field(Obj);
    else if (prim->isByte())
      res = (sint16)field->getInt8Field(Obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  
  jdouble res = 0;

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (jdouble)(sint64)field->getInt8Field(Obj);
    else if (prim->isInt())
      res = (jdouble)(sint64)field->getInt32Field(Obj);
    else if (prim->isShort())
      res = (jdouble)(sint64)field->getInt16Field(Obj);
    else if (prim->isLong())
      res = (jdouble)(sint64)field->getLongField(Obj);
    else if (prim->isChar())
      res = (jdouble)(uint64)field->getInt16Field(Obj);
    else if (prim->isFloat())
      res = (jdouble)field->getFloatField(Obj);
    else if (prim->isDouble())
      res = (jdouble)field->getDoubleField(Obj);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj) {


  jobject result = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, Field);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)Field);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)_obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  JavaObject* res = 0;
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool()) {
      uint8 val =  field->getInt8Field(Obj);
      res = vm->upcalls->boolClass->doNew(vm);
      vm->upcalls->boolValue->setInt8Field(res, val);
    }
    else if (prim->isByte()) {
      sint8 val =  field->getInt8Field(Obj);
      res = vm->upcalls->byteClass->doNew(vm);
      vm->upcalls->byteValue->setInt8Field(res, val);
    }
    else if (prim->isChar()) {
      uint16 val =  field->getInt16Field(Obj);
      res = vm->upcalls->charClass->doNew(vm);
      vm->upcalls->charValue->setInt16Field(res, val);
    }
    else if (prim->isShort()) {
      sint16 val =  field->getInt16Field(Obj);
      res = vm->upcalls->shortClass->doNew(vm);
      vm->upcalls->shortValue->setInt16Field(res, val);
    }
    else if (prim->isInt()) {
      sint32 val =  field->getInt32Field(Obj);
      res = vm->upcalls->intClass->doNew(vm);
      vm->upcalls->intValue->setInt32Field(res, val);
    }
    else if (prim->isLong()) {
      sint64 val =  field->getLongField(Obj);
      res = vm->upcalls->longClass->doNew(vm);
      vm->upcalls->longValue->setLongField(res, val);
    }
    else if (prim->isFloat()) {
      float val =  field->getFloatField(Obj);
      res = vm->upcalls->floatClass->doNew(vm);
      vm->upcalls->floatValue->setFloatField(res, val);
    }
    else if (prim->isDouble()) {
      double val =  field->getDoubleField(Obj);
      res = vm->upcalls->doubleClass->doNew(vm);
      vm->upcalls->doubleValue->setDoubleField(res, val);
    }
  } else {
    res =  field->getObjectField(Obj);
  }
  
  result = (jobject) res;

  END_NATIVE_EXCEPTION

  return (jobject)result;
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jobject val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, Field);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)Field);
  JavaField* field = &(cl->virtualFields[index]);
  uintptr_t buf = (uintptr_t)alloca(sizeof(uint64));
  void* _buf = (void*)buf;
  const Typedef* type = field->getSignature();
  ((JavaObject*)val)->decapsulePrimitive(vm, buf, type);

  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  

  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      field->setInt8Field(Obj, ((uint8*)_buf)[0]);
    else if (prim->isByte())
      field->setInt8Field(Obj, ((sint8*)_buf)[0]);
    else if (prim->isChar())
      field->setInt16Field(Obj, ((uint16*)_buf)[0]);
    else if (prim->isShort())
      field->setInt16Field(Obj, ((sint16*)_buf)[0]);
    else if (prim->isInt())
      field->setInt32Field(Obj, ((sint32*)_buf)[0]);
    else if (prim->isLong())
      field->setLongField(Obj, ((sint64*)_buf)[0]);
    else if (prim->isFloat())
      field->setFloatField(Obj, ((float*)_buf)[0]);
    else if (prim->isDouble())
      field->setDoubleField(Obj, ((double*)_buf)[0]);
  } else {
    field->setObjectField(Obj, ((JavaObject**)_buf)[0]);
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
 
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      field->setInt8Field(Obj, (uint8)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
  
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jbyte val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      field->setInt8Field(Obj, (sint8)val);
    else if (prim->isShort())
      field->setInt16Field(Obj, (sint16)val);
    else if (prim->isInt())
      field->setInt32Field(Obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(Obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(Obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      field->setInt16Field(Obj, (uint16)val);
    else if (prim->isInt())
      field->setInt32Field(Obj, (uint32)val);
    else if (prim->isLong())
      field->setLongField(Obj, (uint64)val);
    else if (prim->isFloat())
      field->setFloatField(Obj, (float)(uint32)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)(uint64)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      field->setInt16Field(Obj, (sint16)val);
    else if (prim->isInt())
      field->setInt32Field(Obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(Obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(Obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isInt())
      field->setInt32Field(Obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(Obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(Obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isLong())
      field->setLongField(Obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(Obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {


  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isFloat())
      field->setFloatField(Obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else 
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, obj);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)obj);
  JavaField* field = &(cl->virtualFields[index]);
  
  void* Obj = (void*)obj;

  if (isStatic(field->access)) {
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isDouble())
      field->setDoubleField(Obj, (double)val);
    else
      vm->illegalArgumentException("");
  } else {
    vm->illegalArgumentException("");
  }

  END_NATIVE_EXCEPTION
}

// Never throws.
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_objectFieldOffset(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Unsafe, jobject Field) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = internalGetClass(vm, Field);
  JavaField* slot = vm->upcalls->fieldSlot;
  uint32 index = (uint32)slot->getInt32Field((JavaObject*)Field);
  JavaField* field = &(cl->virtualFields[index]);
  return (jlong)field->ptrOffset;
}

}
