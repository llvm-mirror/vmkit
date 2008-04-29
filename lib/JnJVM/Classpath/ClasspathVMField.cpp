//===- ClasspathVMField.cpp - GNU classpath java/lang/reflect/Field -------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "ClasspathVMField.h"
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
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)obj).IntVal.getZExtValue());
  return field->access;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)obj).IntVal.getZExtValue());
  JavaObject* loader = field->classDef->classLoader;
  CommonClass* cl = field->signature->assocClass(loader);
  return (jclass)cl->getClassDelegatee();
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dInt && ass != AssessorDesc::dChar && ass != AssessorDesc::dByte && ass != AssessorDesc::dShort)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  return (sint32)(*field)((JavaObject*)obj).IntVal.getZExtValue();
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dInt && ass != AssessorDesc::dChar && ass != AssessorDesc::dByte && ass != AssessorDesc::dShort &&
      ass != AssessorDesc::dLong)
    JavaThread::get()->isolate->illegalArgumentException("");
  
  return (sint64)(*field)((JavaObject*)obj).IntVal.getZExtValue();
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dBool)
    JavaThread::get()->isolate->illegalArgumentException("");
  return (uint8)(*field)((JavaObject*)obj).IntVal.getZExtValue();
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dByte || ass == AssessorDesc::dInt || ass == AssessorDesc::dShort || ass == AssessorDesc::dLong) {
    return (jfloat)(*field)((JavaObject*)obj).IntVal.getSExtValue();
  } else if (ass == AssessorDesc::dChar) {
    return (jfloat)(*field)((JavaObject*)obj).IntVal.getZExtValue();
  } else if (ass == AssessorDesc::dFloat) {
    return (jfloat)(*field)((JavaObject*)obj).FloatVal;
  }
  JavaThread::get()->isolate->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dByte)
    JavaThread::get()->isolate->illegalArgumentException("");
  return (sint8)(*field)((JavaObject*)obj).IntVal.getSExtValue();
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dChar)
    JavaThread::get()->isolate->illegalArgumentException("");
  return (uint16)(*field)((JavaObject*)obj).IntVal.getZExtValue();
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dShort && ass != AssessorDesc::dByte)
    JavaThread::get()->isolate->illegalArgumentException("");
  return (sint16)(*field)((JavaObject*)obj).IntVal.getSExtValue();
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dByte || ass == AssessorDesc::dInt || ass == AssessorDesc::dShort || ass == AssessorDesc::dLong) {
    return (jdouble)(*field)((JavaObject*)obj).IntVal.getSExtValue();
  } else if (ass == AssessorDesc::dChar) {
    return (jdouble)(*field)((JavaObject*)obj).IntVal.getZExtValue();
  } else if (ass == AssessorDesc::dFloat) {
    return (jdouble)(*field)((JavaObject*)obj).FloatVal;
  } else if (ass == AssessorDesc::dDouble) {
    return (jdouble)(*field)((JavaObject*)obj).DoubleVal;
  }
  JavaThread::get()->isolate->illegalArgumentException("");
  return 0.0;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject _obj) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  Typedef* type = field->signature;
  const AssessorDesc* ass = type->funcs;
  JavaObject* obj = (JavaObject*)_obj;
  llvm::GenericValue gv = (*field)(obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  
  JavaObject* res = 0;
  if (ass == AssessorDesc::dBool) {
    res = (*Classpath::boolClass)(vm);
    (*Classpath::boolValue)(res, (uint32)gv.IntVal.getBoolValue());
  } else if (ass == AssessorDesc::dByte) {
    res = (*Classpath::byteClass)(vm);
    (*Classpath::byteValue)(res, (uint32)gv.IntVal.getSExtValue());
  } else if (ass == AssessorDesc::dChar) {
    res = (*Classpath::charClass)(vm);
    (*Classpath::charValue)(res, (uint32)gv.IntVal.getZExtValue());
  } else if (ass == AssessorDesc::dShort) {
    res = (*Classpath::shortClass)(vm);
    (*Classpath::shortValue)(res, (uint32)gv.IntVal.getSExtValue());
  } else if (ass == AssessorDesc::dInt) {
    res = (*Classpath::intClass)(vm);
    (*Classpath::intValue)(res, (uint32)gv.IntVal.getSExtValue());
  } else if (ass == AssessorDesc::dLong) {
    res = (*Classpath::longClass)(vm);
    (*Classpath::longValue)(res, (sint64)gv.IntVal.getSExtValue());
  } else if (ass == AssessorDesc::dFloat) {
    res = (*Classpath::floatClass)(vm);
    (*Classpath::floatValue)(res, gv.FloatVal);
  } else if (ass == AssessorDesc::dDouble) {
    res = (*Classpath::doubleClass)(vm);
    (*Classpath::doubleValue)(res, gv.DoubleVal);
  } else if (ass == AssessorDesc::dTab || ass == AssessorDesc::dRef) {
    res = (JavaObject*)gv.PointerVal;
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
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  void** buf = (void**)alloca(sizeof(uint64));
  void* _buf = (void*)buf;
  NativeUtil::decapsulePrimitive(JavaThread::get()->isolate, buf, (JavaObject*)val, field->signature);

  const AssessorDesc* ass = field->signature->funcs;
  
  if (ass == AssessorDesc::dBool) {
    (*field)((JavaObject*)obj, ((uint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dByte) {
    (*field)((JavaObject*)obj, ((uint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dChar) {
    (*field)((JavaObject*)obj, ((uint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dShort) {
    (*field)((JavaObject*)obj, ((uint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dInt) {
    (*field)((JavaObject*)obj, ((uint32*)_buf)[0]);
  } else if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, ((sint64*)_buf)[0]);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, ((float*)_buf)[0]);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, ((double*)_buf)[0]);
  } else if (ass == AssessorDesc::dTab || ass == AssessorDesc::dRef) {
    (*field)((JavaObject*)obj, ((JavaObject**)_buf)[0]);
  } else {
    JavaThread::get()->isolate->unknownError("should not be here");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jboolean val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass != AssessorDesc::dBool)
    JavaThread::get()->isolate->illegalArgumentException("");
  (*field)((JavaObject*)obj, (uint32)val);
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jbyte val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dByte || ass == AssessorDesc::dShort || ass == AssessorDesc::dInt) {
    (*field)((JavaObject*)obj, (uint32)val);
  } else if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jchar val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dChar || ass == AssessorDesc::dInt) {
    (*field)((JavaObject*)obj, (uint32)val);
  } else if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jshort val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dShort || ass == AssessorDesc::dInt) {
    (*field)((JavaObject*)obj, (uint32)val);
  } else if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jint val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dInt) {
    (*field)((JavaObject*)obj, (uint32)val);
  } else if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jlong val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dLong) {
    (*field)((JavaObject*)obj, (sint64)val);
  } else if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jfloat val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dFloat) {
    (*field)((JavaObject*)obj, (float)val);
  } else if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
  } else {
    JavaThread::get()->isolate->illegalArgumentException("");
  }
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject Field, jobject obj, jdouble val) {
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  const AssessorDesc* ass = field->signature->funcs;
  if (ass == AssessorDesc::dDouble) {
    (*field)((JavaObject*)obj, (double)val);
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
  JavaField* field = (JavaField*)((*Classpath::fieldSlot)((JavaObject*)Field).IntVal.getZExtValue());
  return (jlong)field->ptrOffset;
}

}
