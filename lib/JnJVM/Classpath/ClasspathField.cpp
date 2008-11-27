//===- ClasspathVMField.cpp - GNU classpath java/lang/reflect/Field -------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "JavaClass.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

using namespace jnjvm;

extern "C" {


static UserClass* internalGetClass(Jnjvm* vm, JavaField* field, jobject Field) {
#ifdef ISOLATE_SHARING
  JavaField* slot = vm->upcalls->fieldClass;
  jclass Cl = (jclass)slot->getInt32Field((JavaObject*)Field);
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(vm, Cl, false);
  return cl;
#else
  return field->classDef;
#endif
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)obj);
  return field->access;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)obj);
  UserClass* fieldCl = internalGetClass(vm, field, obj);
  JnjvmClassLoader* loader = fieldCl->classLoader;
  UserCommonClass* cl = field->getSignature()->assocClass(loader);
  return (jclass)cl->getClassDelegatee(vm);
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  const Typedef* type = field->getSignature();
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;

    if (prim->isInt())
      return (sint32)field->getInt32Field(Obj);
    if (prim->isChar())
      return (uint32)field->getInt16Field(Obj);
    if (prim->isByte())
      return (sint32)field->getInt8Field(Obj);
    if (prim->isShort())
      return (sint32)field->getInt16Field(Obj);
  }
  
  vm->illegalArgumentException("");
  return 0;
  
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    
    if (prim->isInt())
      return (sint64)field->getInt32Field(Obj);
    if (prim->isChar())
      return (uint64)field->getInt16Field(Obj);
    if (prim->isByte())
      return (sint64)field->getInt8Field(Obj);
    if (prim->isShort())
      return (sint64)field->getInt16Field(Obj);
    if (prim->isLong())
      return (sint64)field->getLongField(Obj);
  }

  vm->illegalArgumentException("");
  return 0;
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())  
      return (uint8)field->getInt8Field(Obj);
  }
  
  vm->illegalArgumentException("");

  return 0;
  
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      return (jfloat)field->getInt8Field(Obj);
    if (prim->isInt())
      return (jfloat)field->getInt32Field((JavaObject*)obj);
    if (prim->isShort())
      return (jfloat)field->getInt16Field((JavaObject*)obj);
    if (prim->isLong())
      return (jfloat)field->getLongField((JavaObject*)obj);
    if (prim->isChar())
      // Cast to uint32 because char is unsigned.
      return (jfloat)(uint32)field->getInt16Field((JavaObject*)obj);
    if (prim->isFloat())
      return (jfloat)field->getFloatField((JavaObject*)obj);
  }
  
  vm->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      return (sint8)field->getInt8Field(Obj);
  }
  
  vm->illegalArgumentException("");
  
  return 0;
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      return (uint16)field->getInt16Field((JavaObject*)obj);
  }
  
  vm->illegalArgumentException("");
  
  return 0;
  
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      return (sint16)field->getInt16Field(Obj);
    if (prim->isByte())
      return (sint16)field->getInt8Field(Obj);
  }
  
  vm->illegalArgumentException("");
  
  return 0;
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      return (jdouble)(sint64)field->getInt8Field(Obj);
    if (prim->isInt())
      return (jdouble)(sint64)field->getInt32Field(Obj);
    if (prim->isShort())
      return (jdouble)(sint64)field->getInt16Field(Obj);
    if (prim->isLong())
      return (jdouble)(sint64)field->getLongField(Obj);
    if (prim->isChar())
      return (jdouble)(uint64)field->getInt16Field(Obj);
    if (prim->isFloat())
      return (jdouble)field->getFloatField(Obj);
    if (prim->isDouble())
      return (jdouble)field->getDoubleField(Obj);
  }
  
  vm->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)_obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
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

  return (jobject)res;
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jobject val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  uintptr_t buf = (uintptr_t)alloca(sizeof(uint64));
  void* _buf = (void*)buf;
  const Typedef* type = field->getSignature();
  NativeUtil::decapsulePrimitive(vm, buf, (JavaObject*)val, type);

  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  

  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      return field->setInt8Field(Obj, ((uint8*)_buf)[0]);
    if (prim->isByte())
      return field->setInt8Field(Obj, ((sint8*)_buf)[0]);
    if (prim->isChar())
      return field->setInt16Field(Obj, ((uint16*)_buf)[0]);
    if (prim->isShort())
      return field->setInt16Field(Obj, ((sint16*)_buf)[0]);
    if (prim->isInt())
      return field->setInt32Field(Obj, ((sint32*)_buf)[0]);
    if (prim->isLong())
      return field->setLongField(Obj, ((sint64*)_buf)[0]);
    if (prim->isFloat())
      return field->setFloatField(Obj, ((float*)_buf)[0]);
    if (prim->isDouble())
      return field->setDoubleField(Obj, ((double*)_buf)[0]);
  } else {
    return field->setObjectField(Obj, ((JavaObject**)_buf)[0]);
  }

  // Unreachable code
  return;
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
 
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      return field->setInt8Field(Obj, (uint8)val);
  }

  vm->illegalArgumentException("");
  
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jbyte val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      return field->setInt8Field(Obj, (sint8)val);
    if (prim->isShort())
      return field->setInt16Field(Obj, (sint16)val);
    if (prim->isInt())
      return field->setInt32Field(Obj, (sint32)val);
    if (prim->isLong())
      return field->setLongField(Obj, (sint64)val);
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      return field->setInt16Field(Obj, (uint16)val);
    if (prim->isInt())
      return field->setInt32Field(Obj, (uint32)val);
    if (prim->isLong())
      return field->setLongField(Obj, (uint64)val);
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)(uint32)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)(uint64)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      return field->setInt16Field(Obj, (sint16)val);
    if (prim->isInt())
      return field->setInt32Field(Obj, (sint32)val);
    if (prim->isLong())
      return field->setLongField(Obj, (sint64)val);
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isInt())
      return field->setInt32Field(Obj, (sint32)val);
    if (prim->isLong())
      return field->setLongField(Obj, (sint64)val);
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isLong())
      return field->setLongField(Obj, (sint64)val);
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isFloat())
      return field->setFloatField(Obj, (float)val);
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
 
  vm->illegalArgumentException("");
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  
  void* Obj = (void*)obj;

  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  } else {
    verifyNull(Obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isDouble())
      return field->setDoubleField(Obj, (double)val);
  }
  
  vm->illegalArgumentException("");
}

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_objectFieldOffset(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* Unsafe,
JavaObject* Field) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* slot = vm->upcalls->fieldSlot;
  JavaField* field = (JavaField*)slot->getInt32Field((JavaObject*)Field);
  return (jlong)field->ptrOffset;
}

}
