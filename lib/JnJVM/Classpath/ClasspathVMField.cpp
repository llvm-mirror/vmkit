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
  JavaObject* loader = field->classDef->classLoader;
  CommonClass* cl = field->signature->assocClass(loader);
  return (jclass)cl->getClassDelegatee();
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dInt && ass != AssessorDesc::dChar && ass != AssessorDesc::dByte && ass != AssessorDesc::dShort)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);

  return isStatic(field->access) ? 
    (sint32)field->getStaticInt32Field() :
    (sint32)field->getVirtualInt32Field((JavaObject*)obj);
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dInt && ass != AssessorDesc::dChar && ass != AssessorDesc::dByte && ass != AssessorDesc::dShort &&
      ass != AssessorDesc::dLong)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  return isStatic(field->access) ? 
    (sint64)field->getStaticLongField() :
    (sint64)field->getVirtualLongField((JavaObject*)obj);
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (ass != AssessorDesc::dBool)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  return isStatic(field->access) ? 
    (uint8)field->getStaticInt8Field() :
    (uint8)field->getVirtualInt8Field((JavaObject*)obj);
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dByte) {
    return isStatic(field->access) ? 
      (jfloat)field->getStaticInt8Field() :
      (jfloat)field->getVirtualInt8Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dInt) {
    return isStatic(field->access) ? 
      (jfloat)field->getStaticInt32Field() :
      (jfloat)field->getVirtualInt32Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dShort) {
    return isStatic(field->access) ? 
      (jfloat)field->getStaticInt16Field() :
      (jfloat)field->getVirtualInt16Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dLong) {
    return isStatic(field->access) ? 
      (jfloat)field->getStaticLongField() :
      (jfloat)field->getVirtualLongField((JavaObject*)obj);
  } else if (ass == AssessorDesc::dChar) {
    return isStatic(field->access) ? 
      (jfloat)(uint32)field->getStaticInt16Field() :
      (jfloat)(uint32)field->getVirtualInt16Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dFloat) {
    return isStatic(field->access) ? 
      (jfloat)field->getStaticFloatField() :
      (jfloat)field->getVirtualFloatField((JavaObject*)obj);
  }
  JavaThread::get()->isolate->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dByte)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  return isStatic(field->access) ? 
    (sint8)field->getStaticInt8Field() :
    (sint8)field->getVirtualInt8Field((JavaObject*)obj);
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (ass != AssessorDesc::dChar)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  return isStatic(field->access) ? 
    (uint16)field->getStaticInt16Field() :
    (uint16)field->getVirtualInt16Field((JavaObject*)obj);
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (ass != AssessorDesc::dShort && ass != AssessorDesc::dByte)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  return isStatic(field->access) ? 
    (sint16)field->getStaticInt16Field() :
    (sint16)field->getVirtualInt16Field((JavaObject*)obj);
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dByte) {
    return isStatic(field->access) ? 
      (jdouble)(sint64)field->getStaticInt8Field() :
      (jdouble)(sint64)field->getVirtualInt8Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dInt) {
    return isStatic(field->access) ? 
      (jdouble)(sint64)field->getStaticInt32Field() :
      (jdouble)(sint64)field->getVirtualInt32Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dShort) {
    return isStatic(field->access) ? 
      (jdouble)(sint64)field->getStaticInt16Field() :
      (jdouble)(sint64)field->getVirtualInt16Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dLong) {
    return isStatic(field->access) ? 
      (jdouble)(sint64)field->getStaticLongField() :
      (jdouble)(sint64)field->getVirtualLongField((JavaObject*)obj);
  } else if (ass == AssessorDesc::dChar) {
    return isStatic(field->access) ? 
      (jdouble)(uint64)field->getStaticInt16Field() :
      (jdouble)(uint64)field->getVirtualInt16Field((JavaObject*)obj);
  } else if (ass == AssessorDesc::dFloat) {
    return isStatic(field->access) ? 
      (jdouble)field->getStaticFloatField() :
      (jdouble)field->getVirtualFloatField((JavaObject*)obj);
  } else if (ass == AssessorDesc::dDouble) {
    return isStatic(field->access) ? 
      (jdouble)field->getStaticDoubleField() :
      (jdouble)field->getVirtualDoubleField((JavaObject*)obj);
  }
  JavaThread::get()->isolate->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  Typedef* type = field->signature;
  const AssessorDesc* ass = type->funcs;
  JavaObject* obj = (JavaObject*)_obj;
  Jnjvm* vm = JavaThread::get()->isolate;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  JavaObject* res = 0;
  if (ass == AssessorDesc::dBool) {
    uint8 val =  (isStatic(field->access) ? 
      field->getStaticInt8Field() :
      field->getVirtualInt8Field(obj));
    res = (*Classpath::boolClass)(vm);
    Classpath::boolValue->setVirtualInt8Field(res, val);
  } else if (ass == AssessorDesc::dByte) {
    sint8 val =  (isStatic(field->access) ? 
      field->getStaticInt8Field() :
      field->getVirtualInt8Field(obj));
    res = (*Classpath::byteClass)(vm);
    Classpath::byteValue->setVirtualInt8Field(res, val);
  } else if (ass == AssessorDesc::dChar) {
    uint16 val =  (isStatic(field->access) ? 
      field->getStaticInt16Field() :
      field->getVirtualInt16Field(obj));
    res = (*Classpath::charClass)(vm);
    Classpath::charValue->setVirtualInt16Field(res, val);
  } else if (ass == AssessorDesc::dShort) {
    sint16 val =  (isStatic(field->access) ? 
      field->getStaticInt16Field() :
      field->getVirtualInt16Field(obj));
    res = (*Classpath::shortClass)(vm);
    Classpath::shortValue->setVirtualInt16Field(res, val);
  } else if (ass == AssessorDesc::dInt) {
    sint32 val =  (isStatic(field->access) ? 
      field->getStaticInt32Field() :
      field->getVirtualInt32Field(obj));
    res = (*Classpath::intClass)(vm);
    Classpath::intValue->setVirtualInt32Field(res, val);
  } else if (ass == AssessorDesc::dLong) {
    sint64 val =  (isStatic(field->access) ? 
      field->getStaticLongField() :
      field->getVirtualLongField(obj));
    res = (*Classpath::longClass)(vm);
    Classpath::longValue->setVirtualLongField(res, val);
  } else if (ass == AssessorDesc::dFloat) {
    float val =  (isStatic(field->access) ? 
      field->getStaticFloatField() :
      field->getVirtualFloatField(obj));
    res = (*Classpath::floatClass)(vm);
    Classpath::floatValue->setVirtualFloatField(res, val);
  } else if (ass == AssessorDesc::dDouble) {
    double val =  (isStatic(field->access) ? 
      field->getStaticDoubleField() :
      field->getVirtualDoubleField(obj));
    res = (*Classpath::doubleClass)(vm);
    Classpath::doubleValue->setVirtualDoubleField(res, val);
  } else if (ass == AssessorDesc::dTab || ass == AssessorDesc::dRef) {
    res =  (isStatic(field->access) ? 
      field->getStaticObjectField() :
      field->getVirtualObjectField(obj));
  } else {
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
  NativeUtil::decapsulePrimitive(JavaThread::get()->isolate, buf, (JavaObject*)val, field->signature);

  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dBool) {
    isStatic(field->access) ? 
      field->setStaticInt8Field(((uint8*)_buf)[0]) :
      field->setVirtualInt8Field((JavaObject*)obj, ((uint8*)_buf)[0]);
  } else if (ass == AssessorDesc::dByte) {
    isStatic(field->access) ? 
      field->setStaticInt8Field(((sint8*)_buf)[0]) :
      field->setVirtualInt8Field((JavaObject*)obj, ((sint8*)_buf)[0]);
  } else if (ass == AssessorDesc::dChar) {
    isStatic(field->access) ? 
      field->setStaticInt16Field(((uint16*)_buf)[0]) :
      field->setVirtualInt16Field((JavaObject*)obj, ((uint16*)_buf)[0]);
  } else if (ass == AssessorDesc::dShort) {
    isStatic(field->access) ? 
      field->setStaticInt16Field(((sint16*)_buf)[0]) :
      field->setVirtualInt16Field((JavaObject*)obj, ((sint16*)_buf)[0]);
  } else if (ass == AssessorDesc::dInt) {
    isStatic(field->access) ? 
      field->setStaticInt32Field(((sint32*)_buf)[0]) :
      field->setVirtualInt32Field((JavaObject*)obj, ((sint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField(((sint64*)_buf)[0]) :
      field->setVirtualLongField((JavaObject*)obj, ((sint64*)_buf)[0]);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField(((float*)_buf)[0]) :
      field->setVirtualFloatField((JavaObject*)obj, ((float*)_buf)[0]);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField(((double*)_buf)[0]) :
      field->setVirtualDoubleField((JavaObject*)obj, ((double*)_buf)[0]);
  } else if (ass == AssessorDesc::dTab || ass == AssessorDesc::dRef) {
    isStatic(field->access) ? 
      field->setStaticObjectField(((JavaObject**)_buf)[0]) :
      field->setVirtualObjectField((JavaObject*)obj, ((JavaObject**)_buf)[0]);
  } else {
    JavaThread::get()->isolate->unknownError("should not be here");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (ass != AssessorDesc::dBool)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  isStatic(field->access) ? 
    field->setStaticInt8Field((uint8)val) :
    field->setVirtualInt8Field((JavaObject*)obj, (uint8)val);
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj, jbyte val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  JavaObject* obj = (JavaObject*)_obj;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dByte) {
    isStatic(field->access) ? 
      field->setStaticInt8Field((sint8)val) :
      field->setVirtualInt8Field((JavaObject*)obj, (sint8)val);
  } else if (ass == AssessorDesc::dShort) {
    isStatic(field->access) ? 
      field->setStaticInt16Field((sint16)val) :
      field->setVirtualInt16Field((JavaObject*)obj, (sint16)val);
  } else if (ass == AssessorDesc::dInt) {
    isStatic(field->access) ? 
      field->setStaticInt32Field((sint32)val) :
      field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
  } else if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField((sint64)val) :
      field->setVirtualLongField((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dChar) {
    isStatic(field->access) ? 
      field->setStaticInt16Field((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (uint16)val);
  } else if (ass == AssessorDesc::dInt) {
    isStatic(field->access) ? 
      field->setStaticInt32Field((uint32)val) :
      field->setVirtualInt32Field((JavaObject*)obj, (uint32)val);
  } else if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField((uint64)val) :
      field->setVirtualLongField((JavaObject*)obj, (uint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)(uint32)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)(uint32)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)(uint64)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)(uint64)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dShort) {
    isStatic(field->access) ? 
      field->setStaticInt16Field((sint16)val) :
      field->setVirtualInt16Field((JavaObject*)obj, (sint16)val);
  } else if (ass == AssessorDesc::dInt) {
    isStatic(field->access) ? 
      field->setStaticInt32Field((sint32)val) :
      field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
  } else if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField((sint64)val) :
      field->setVirtualLongField((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dInt) {
    isStatic(field->access) ? 
      field->setStaticInt32Field((sint32)val) :
      field->setVirtualInt32Field((JavaObject*)obj, (sint32)val);
  } else if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField((sint64)val) :
      field->setVirtualLongField((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dLong) {
    isStatic(field->access) ? 
      field->setStaticLongField((sint64)val) :
      field->setVirtualLongField((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dFloat) {
    isStatic(field->access) ? 
      field->setStaticFloatField((float)val) :
      field->setVirtualFloatField((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {
  JavaField* field = (JavaField*)Classpath::fieldSlot->getVirtualInt32Field((JavaObject*)Field);
  const AssessorDesc* ass = field->signature->funcs;
  
  if (isStatic(field->access)) 
    JavaThread::get()->isolate->initialiseClass(field->classDef);
  
  if (ass == AssessorDesc::dDouble) {
    isStatic(field->access) ? 
      field->setStaticDoubleField((double)val) :
      field->setVirtualDoubleField((JavaObject*)obj, (double)val);
  } else {
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
