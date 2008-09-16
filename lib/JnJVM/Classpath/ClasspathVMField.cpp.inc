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
#ifdef MULTIPLE_VM
  jclass Cl = (jclass)vm->upcalls->fieldClass->getInt32Field((JavaObject*)Field);
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(Cl, false);
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
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)obj);
  return field->access;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)obj);
#ifdef MULTIPLE_VM
  jclass Cl = (jclass)vm->upcalls->fieldClass->getInt32Field((JavaObject*)obj);
  UserClass* fieldCl = (UserClass*)NativeUtil::resolvedImplClass(Cl, false);
  JnjvmClassLoader* loader = fieldCl->classLoader;
#else
  JnjvmClassLoader* loader = field->classDef->classLoader;
#endif
  UserCommonClass* cl = field->getSignature()->assocClass(loader);
  return (jclass)cl->getClassDelegatee(vm);
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case INT_ID :
      return (sint32)field->getInt32Field(Obj);
    case CHAR_ID :
      return (uint32)field->getInt16Field(Obj);
    case BYTE_ID :
      return (sint32)field->getInt8Field(Obj);
    case SHORT_ID :
      return (sint32)field->getInt16Field(Obj);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  return 0;
  
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case INT_ID :
      return (sint64)field->getInt32Field(Obj);
    case CHAR_ID :
      return (uint64)field->getInt16Field(Obj);
    case BYTE_ID :
      return (sint64)field->getInt8Field(Obj);
    case SHORT_ID :
      return (sint64)field->getInt16Field(Obj);
    case LONG_ID :
      return (sint64)field->getLongField(Obj);
    default:
      JavaThread::get()->isolate->illegalArgumentException("");     
  }
  return 0;
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case BOOL_ID :
      return (uint8)field->getInt8Field(Obj);
    default:
      JavaThread::get()->isolate->illegalArgumentException("");
  }

  return 0;
  
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) { 
    case BYTE_ID :
      return (jfloat)field->getInt8Field(Obj);
    case INT_ID :
      return (jfloat)field->getInt32Field((JavaObject*)obj);
    case SHORT_ID :
      return (jfloat)field->getInt16Field((JavaObject*)obj);
    case LONG_ID :
      return (jfloat)field->getLongField((JavaObject*)obj);
    case CHAR_ID :
      return (jfloat)(uint32)field->getInt16Field((JavaObject*)obj);
    case FLOAT_ID :
      return (jfloat)field->getFloatField((JavaObject*)obj);
    default:
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  return 0.0;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case BYTE_ID :
      return (sint8)field->getInt8Field(Obj);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  
  return 0;
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case CHAR_ID :
      return (uint16)field->getInt16Field((JavaObject*)obj);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  
  return 0;
  
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case SHORT_ID :
      return (sint16)field->getInt16Field(Obj);
    case BYTE_ID :
      return (sint16)field->getInt8Field(Obj);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  
  return 0;
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case BYTE_ID :
      return (jdouble)(sint64)field->getInt8Field(Obj);
    case INT_ID :
      return (jdouble)(sint64)field->getInt32Field(Obj);
    case SHORT_ID :
      return (jdouble)(sint64)field->getInt16Field(Obj);
    case LONG_ID :
      return (jdouble)(sint64)field->getLongField(Obj);
    case CHAR_ID :
      return (jdouble)(uint64)field->getInt16Field(Obj);
    case FLOAT_ID :
      return (jdouble)field->getFloatField(Obj);
    case DOUBLE_ID :
      return (jdouble)field->getDoubleField(Obj);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  return 0.0;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  Typedef* type = field->getSignature();
  const AssessorDesc* ass = type->funcs;
  
  JavaObject* Obj = (JavaObject*)_obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  JavaObject* res = 0;
  switch (ass->numId) {
    case BOOL_ID : {
      uint8 val =  field->getInt8Field(Obj);
      res = vm->upcalls->boolClass->doNew(vm);
      vm->upcalls->boolValue->setInt8Field(res, val);
      break;
    }
    case BYTE_ID : {
      sint8 val =  field->getInt8Field(Obj);
      res = vm->upcalls->byteClass->doNew(vm);
      vm->upcalls->byteValue->setInt8Field(res, val);
      break;
    }
    case CHAR_ID : {
      uint16 val =  field->getInt16Field(Obj);
      res = vm->upcalls->charClass->doNew(vm);
      vm->upcalls->charValue->setInt16Field(res, val);
      break;
    }
    case SHORT_ID : {
      sint16 val =  field->getInt16Field(Obj);
      res = vm->upcalls->shortClass->doNew(vm);
      vm->upcalls->shortValue->setInt16Field(res, val);
      break;
    }
    case INT_ID : {
      sint32 val =  field->getInt32Field(Obj);
      res = vm->upcalls->intClass->doNew(vm);
      vm->upcalls->intValue->setInt32Field(res, val);
      break;
    }
    case LONG_ID : {
      sint64 val =  field->getLongField(Obj);
      res = vm->upcalls->longClass->doNew(vm);
      vm->upcalls->longValue->setLongField(res, val);
      break;
    }
    case FLOAT_ID : {
      float val =  field->getFloatField(Obj);
      res = vm->upcalls->floatClass->doNew(vm);
      vm->upcalls->floatValue->setFloatField(res, val);
      break;
    }
    case DOUBLE_ID : {
      double val =  field->getDoubleField(Obj);
      res = vm->upcalls->doubleClass->doNew(vm);
      vm->upcalls->doubleValue->setDoubleField(res, val);
      break;
    }
    case OBJECT_ID :
    case ARRAY_ID :
      res =  field->getObjectField(Obj);
      break;
    default:
      JavaThread::get()->isolate->unknownError("should not be here");
  }
  return (jobject)res;
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jobject val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  void** buf = (void**)alloca(sizeof(uint64));
  void* _buf = (void*)buf;
  NativeUtil::decapsulePrimitive(JavaThread::get()->isolate, buf, (JavaObject*)val, field->getSignature());

  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case BOOL_ID :
      return field->setInt8Field(Obj, ((uint8*)_buf)[0]);
    case BYTE_ID :
      return field->setInt8Field(Obj, ((sint8*)_buf)[0]);
    case CHAR_ID :
      return field->setInt16Field(Obj, ((uint16*)_buf)[0]);
    case SHORT_ID :
      return field->setInt16Field(Obj, ((sint16*)_buf)[0]);
    case INT_ID :
      return field->setInt32Field(Obj, ((sint32*)_buf)[0]);
    case LONG_ID :
      return field->setLongField(Obj, ((sint64*)_buf)[0]);
    case FLOAT_ID :
      return field->setFloatField(Obj, ((float*)_buf)[0]);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, ((double*)_buf)[0]);
    case ARRAY_ID :
    case OBJECT_ID :
      return field->setObjectField(Obj, ((JavaObject**)_buf)[0]);
    default :
      JavaThread::get()->isolate->unknownError("should not be here");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
 
  switch (ass->numId) {
    case BOOL_ID :
      return field->setInt8Field(Obj, (uint8)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jbyte val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case BYTE_ID :
      return field->setInt8Field(Obj, (sint8)val);
    case SHORT_ID :
      return field->setInt16Field(Obj, (sint16)val);
    case INT_ID :
      return field->setInt32Field(Obj, (sint32)val);
    case LONG_ID :
      return field->setLongField(Obj, (sint64)val);
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case CHAR_ID :
      return field->setInt16Field(Obj, (uint16)val);
    case INT_ID :
      return field->setInt32Field(Obj, (uint32)val);
    case LONG_ID :
      return field->setLongField(Obj, (uint64)val);
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)(uint32)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)(uint64)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case SHORT_ID :
      return field->setInt16Field(Obj, (sint16)val);
    case INT_ID :
      return field->setInt32Field(Obj, (sint32)val);
    case LONG_ID :
      return field->setLongField(Obj, (sint64)val);
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case INT_ID :
      return field->setInt32Field(Obj, (sint32)val);
    case LONG_ID :
      return field->setLongField(Obj, (sint64)val);
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }

  switch (ass->numId) {
    case LONG_ID :
      return field->setLongField(Obj, (sint64)val);
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* Obj = (JavaObject*)obj;
  
  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case FLOAT_ID :
      return field->setFloatField(Obj, (float)val);
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* Obj = (JavaObject*)obj;

  if (isStatic(field->access)) {
    UserClass* cl = internalGetClass(vm, field, Field);
    cl->initialiseClass(vm);
    Obj = cl->getStaticInstance();
  }
  
  switch (ass->numId) {
    case DOUBLE_ID :
      return field->setDoubleField(Obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_objectFieldOffset(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* Unsafe,
JavaObject* Field) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaField* field = (JavaField*)vm->upcalls->fieldSlot->getInt32Field((JavaObject*)Field);
  return (jlong)field->ptrOffset;
}

}
