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

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)obj);
  return field->access;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)obj);
  JnjvmClassLoader* loader = field->classDef->classLoader;
  CommonClass* cl = field->getSignature()->assocClass(loader);
  return (jclass)cl->getClassDelegatee();
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case INT_ID :
      return isStatic(field->access) ? 
        (sint32)field->getStaticInt32Field() :
        (sint32)field->getVirtualInt32Field((JavaObject*)obj);
    case CHAR_ID :
      return isStatic(field->access) ? 
        (uint32)field->getStaticInt16Field() :
        (uint32)field->getVirtualInt16Field((JavaObject*)obj);
    case BYTE_ID :
      return isStatic(field->access) ? 
        (sint32)field->getStaticInt8Field() :
        (sint32)field->getVirtualInt8Field((JavaObject*)obj);
    case SHORT_ID :
      return isStatic(field->access) ? 
        (sint32)field->getStaticInt16Field() :
        (sint32)field->getVirtualInt16Field((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case INT_ID :
      return isStatic(field->access) ? 
        (sint64)field->getStaticInt32Field() :
        (sint64)field->getVirtualInt32Field((JavaObject*)obj);
    case CHAR_ID :
      return isStatic(field->access) ? 
        (uint64)field->getStaticInt16Field() :
        (uint64)field->getVirtualInt16Field((JavaObject*)obj);
    case BYTE_ID :
      return isStatic(field->access) ? 
        (sint64)field->getStaticInt8Field() :
        (sint64)field->getVirtualInt8Field((JavaObject*)obj);
    case SHORT_ID :
      return isStatic(field->access) ? 
        (sint64)field->getStaticInt16Field() :
        (sint64)field->getVirtualInt16Field((JavaObject*)obj);
    case LONG_ID :
      return isStatic(field->access) ? 
        (sint64)field->getStaticLongField() :
        (sint64)field->getVirtualLongField((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case BOOL_ID :
      return isStatic(field->access) ? 
        (uint8)field->getStaticInt8Field() :
        (uint8)field->getVirtualInt8Field((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) { 
    case BYTE_ID :
      return isStatic(field->access) ? 
        (jfloat)field->getStaticInt8Field() :
        (jfloat)field->getVirtualInt8Field((JavaObject*)obj);
    case INT_ID :
      return isStatic(field->access) ? 
        (jfloat)field->getStaticInt32Field() :
        (jfloat)field->getVirtualInt32Field((JavaObject*)obj);
    case SHORT_ID :
      return isStatic(field->access) ? 
        (jfloat)field->getStaticInt16Field() :
        (jfloat)field->getVirtualInt16Field((JavaObject*)obj);
    case LONG_ID :
      return isStatic(field->access) ? 
        (jfloat)field->getStaticLongField() :
        (jfloat)field->getVirtualLongField((JavaObject*)obj);
    case CHAR_ID :
      return isStatic(field->access) ? 
        (jfloat)(uint32)field->getStaticInt16Field() :
        (jfloat)(uint32)field->getVirtualInt16Field((JavaObject*)obj);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        (jfloat)field->getStaticFloatField() :
        (jfloat)field->getVirtualFloatField((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case BYTE_ID :
      return isStatic(field->access) ? 
        (sint8)field->getStaticInt8Field() :
        (sint8)field->getVirtualInt8Field((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case CHAR_ID :
      return isStatic(field->access) ? 
        (uint16)field->getStaticInt16Field() :
        (uint16)field->getVirtualInt16Field((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case SHORT_ID :
      return isStatic(field->access) ? 
        (sint16)field->getStaticInt16Field() :
        (sint16)field->getVirtualInt16Field((JavaObject*)obj);
    case BYTE_ID :
      return isStatic(field->access) ? 
        (sint16)field->getStaticInt8Field() :
        (sint16)field->getVirtualInt8Field((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case BYTE_ID :
      return isStatic(field->access) ? 
        (jdouble)(sint64)field->getStaticInt8Field() :
        (jdouble)(sint64)field->getVirtualInt8Field((JavaObject*)obj);
    case INT_ID :
      return isStatic(field->access) ? 
        (jdouble)(sint64)field->getStaticInt32Field() :
        (jdouble)(sint64)field->getVirtualInt32Field((JavaObject*)obj);
    case SHORT_ID :
      return isStatic(field->access) ? 
        (jdouble)(sint64)field->getStaticInt16Field() :
        (jdouble)(sint64)field->getVirtualInt16Field((JavaObject*)obj);
    case LONG_ID :
      return isStatic(field->access) ? 
        (jdouble)(sint64)field->getStaticLongField() :
        (jdouble)(sint64)field->getVirtualLongField((JavaObject*)obj);
    case CHAR_ID :
      return isStatic(field->access) ? 
        (jdouble)(uint64)field->getStaticInt16Field() :
        (jdouble)(uint64)field->getVirtualInt16Field((JavaObject*)obj);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        (jdouble)field->getStaticFloatField() :
        (jdouble)field->getVirtualFloatField((JavaObject*)obj);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        (jdouble)field->getStaticDoubleField() :
        (jdouble)field->getVirtualDoubleField((JavaObject*)obj);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  Typedef* type = field->getSignature();
  const AssessorDesc* ass = type->funcs;
  JavaObject* obj = (JavaObject*)_obj;
  Jnjvm* vm = JavaThread::get()->isolate;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  JavaObject* res = 0;
  switch (ass->numId) {
    case BOOL_ID : {
      uint8 val =  (isStatic(field->access) ? 
        field->getStaticInt8Field() :
        field->getVirtualInt8Field(obj));
      res = Classpath::boolClass->doNew(vm);
      Classpath::boolValue->setVirtualInt8Field(res, val);
      break;
    }
    case BYTE_ID : {
      sint8 val =  (isStatic(field->access) ? 
        field->getStaticInt8Field() :
        field->getVirtualInt8Field(obj));
      res = Classpath::byteClass->doNew(vm);
      Classpath::byteValue->setVirtualInt8Field(res, val);
      break;
    }
    case CHAR_ID : {
      uint16 val =  (isStatic(field->access) ? 
        field->getStaticInt16Field() :
        field->getVirtualInt16Field(obj));
      res = Classpath::charClass->doNew(vm);
      Classpath::charValue->setVirtualInt16Field(res, val);
      break;
    }
    case SHORT_ID : {
      sint16 val =  (isStatic(field->access) ? 
        field->getStaticInt16Field() :
        field->getVirtualInt16Field(obj));
      res = Classpath::shortClass->doNew(vm);
      Classpath::shortValue->setVirtualInt16Field(res, val);
      break;
    }
    case INT_ID : {
      sint32 val =  (isStatic(field->access) ? 
        field->getStaticInt32Field() :
        field->getVirtualInt32Field(obj));
      res = Classpath::intClass->doNew(vm);
      Classpath::intValue->setVirtualInt32Field(res, val);
      break;
    }
    case LONG_ID : {
      sint64 val =  (isStatic(field->access) ? 
        field->getStaticLongField() :
        field->getVirtualLongField(obj));
      res = Classpath::longClass->doNew(vm);
      Classpath::longValue->setVirtualLongField(res, val);
      break;
    }
    case FLOAT_ID : {
      float val =  (isStatic(field->access) ? 
        field->getStaticFloatField() :
        field->getVirtualFloatField(obj));
      res = Classpath::floatClass->doNew(vm);
      Classpath::floatValue->setVirtualFloatField(res, val);
      break;
    }
    case DOUBLE_ID : {
      double val =  (isStatic(field->access) ? 
        field->getStaticDoubleField() :
        field->getVirtualDoubleField(obj));
      res = Classpath::doubleClass->doNew(vm);
      Classpath::doubleValue->setVirtualDoubleField(res, val);
      break;
    }
    case OBJECT_ID :
    case ARRAY_ID :
      res =  (isStatic(field->access) ? 
        field->getStaticObjectField() :
        field->getVirtualObjectField(obj));
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  void** buf = (void**)alloca(sizeof(uint64));
  void* _buf = (void*)buf;
  NativeUtil::decapsulePrimitive(JavaThread::get()->isolate, buf, (JavaObject*)val, field->getSignature());

  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case BOOL_ID :
      return isStatic(field->access) ? 
        field->setStaticInt8Field(((uint8*)_buf)[0]) :
        field->setVirtualInt8Field((JavaObject*)obj, ((uint8*)_buf)[0]);
    case BYTE_ID :
      return isStatic(field->access) ? 
        field->setStaticInt8Field(((sint8*)_buf)[0]) :
        field->setVirtualInt8Field((JavaObject*)obj, ((sint8*)_buf)[0]);
    case CHAR_ID :
      return isStatic(field->access) ? 
        field->setStaticInt16Field(((uint16*)_buf)[0]) :
        field->setVirtualInt16Field((JavaObject*)obj, ((uint16*)_buf)[0]);
    case SHORT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt16Field(((sint16*)_buf)[0]) :
        field->setVirtualInt16Field((JavaObject*)obj, ((sint16*)_buf)[0]);
    case INT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt32Field(((sint32*)_buf)[0]) :
        field->setVirtualInt32Field((JavaObject*)obj, ((sint32*)_buf)[0]);
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField(((sint64*)_buf)[0]) :
        field->setVirtualLongField((JavaObject*)obj, ((sint64*)_buf)[0]);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField(((float*)_buf)[0]) :
        field->setVirtualFloatField((JavaObject*)obj, ((float*)_buf)[0]);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField(((double*)_buf)[0]) :
        field->setVirtualDoubleField((JavaObject*)obj, ((double*)_buf)[0]);
    case ARRAY_ID :
    case OBJECT_ID :
      return isStatic(field->access) ? 
        field->setStaticObjectField(((JavaObject**)_buf)[0]) :
        field->setVirtualObjectField((JavaObject*)obj, ((JavaObject**)_buf)[0]);
    default :
      JavaThread::get()->isolate->unknownError("should not be here");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
 
  switch (ass->numId) {
    case BOOL_ID :
      return isStatic(field->access) ? 
        field->setStaticInt8Field((uint8)val) :
        field->setVirtualInt8Field((JavaObject*)obj, (uint8)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
  
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj, jbyte val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  JavaObject* obj = (JavaObject*)_obj;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case BYTE_ID :
      return isStatic(field->access) ? 
        field->setStaticInt8Field((sint8)val) :
        field->setVirtualInt8Field((JavaObject*)obj, (sint8)val);
    case SHORT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt16Field((sint16)val) :
        field->setVirtualInt16Field((JavaObject*)obj, (sint16)val);
    case INT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt32Field((sint32)val) :
        field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField((sint64)val) :
        field->setVirtualLongField((JavaObject*)obj, (sint64)val);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case CHAR_ID :
      return isStatic(field->access) ? 
        field->setStaticInt16Field((uint16)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (uint16)val);
    case INT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt32Field((uint32)val) :
        field->setVirtualInt32Field((JavaObject*)obj, (uint32)val);
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField((uint64)val) :
        field->setVirtualLongField((JavaObject*)obj, (uint64)val);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)(uint32)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)(uint32)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)(uint64)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)(uint64)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case SHORT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt16Field((sint16)val) :
        field->setVirtualInt16Field((JavaObject*)obj, (sint16)val);
    case INT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt32Field((sint32)val) :
        field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField((sint64)val) :
        field->setVirtualLongField((JavaObject*)obj, (sint64)val);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case INT_ID :
      return isStatic(field->access) ? 
        field->setStaticInt32Field((sint32)val) :
        field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField((sint64)val) :
        field->setVirtualLongField((JavaObject*)obj, (sint64)val);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
    default :
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case LONG_ID :
      return isStatic(field->access) ? 
        field->setStaticLongField((sint64)val) :
        field->setVirtualLongField((JavaObject*)obj, (sint64)val);
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case FLOAT_ID :
      return isStatic(field->access) ? 
        field->setStaticFloatField((float)val) :
        field->setVirtualFloatField((JavaObject*)obj, (float)val);
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
    default :
      JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->getSignature()->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  switch (ass->numId) {
    case DOUBLE_ID :
      return isStatic(field->access) ? 
        field->setStaticDoubleField((double)val) :
        field->setVirtualDoubleField((JavaObject*)obj, (double)val);
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
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  return (jlong)field->ptrOffset;
}

}
