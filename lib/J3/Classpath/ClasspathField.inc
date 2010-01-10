//===- ClasspathVMField.cpp - GNU classpath java/lang/reflect/Field -------===//
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
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;

extern "C" {


JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field) {
  jint res = 0;

  llvm_gcroot(Field, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaField* field = Field->getInternalField();
  res = field->access;

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Field_getType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field) {
  
  JavaObject* res = 0;

  llvm_gcroot(Field, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  JnjvmClassLoader* loader = cl->classLoader;
  UserCommonClass* fieldCl = field->getSignature()->assocClass(loader);
  res = fieldCl->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Field_getInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {
  
  jint res = 0;
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  const Typedef* type = field->getSignature();
  bool stat =  isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;

    if (prim->isInt())
      res = (sint32)field->getInt32Field(stat ? StatPtr : obj);
    else if (prim->isChar())
      res = (uint32)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isByte())
      res = (sint32)field->getInt8Field(stat ? StatPtr : obj);
    else if (prim->isShort())
      res = (sint32)field->getInt16Field(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
      vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION
  
  return res;
  
}

JNIEXPORT jlong JNICALL Java_java_lang_reflect_Field_getLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {

  jlong res = 0;
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat =  isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    
    if (prim->isInt())
      res = (sint64)field->getInt32Field(stat ? StatPtr : obj);
    else if (prim->isChar())
      res = (uint64)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isByte())
      res = (sint64)field->getInt8Field(stat ? StatPtr : obj);
    else if (prim->isShort())
      res = (sint64)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isLong())
      res = (sint64)field->getLongField(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jboolean JNICALL Java_java_lang_reflect_Field_getBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {

  jboolean res = 0;
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat =  isStatic(field->access);
  void* StatPtr = 0;
   
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())  
      res = (uint8)field->getInt8Field(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION

  return res;
  
}

JNIEXPORT jfloat JNICALL Java_java_lang_reflect_Field_getFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {
  
  jfloat res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (jfloat)field->getInt8Field(stat ? StatPtr : obj);
    else if (prim->isInt())
      res = (jfloat)field->getInt32Field(stat ? StatPtr : obj);
    else if (prim->isShort())
      res = (jfloat)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isLong())
      res = (jfloat)field->getLongField(stat ? StatPtr : obj);
    else if (prim->isChar())
      // Cast to uint32 because char is unsigned.
      res = (jfloat)(uint32)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isFloat())
      res = (jfloat)field->getFloatField(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jbyte JNICALL Java_java_lang_reflect_Field_getByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {

  jbyte res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (sint8)field->getInt8Field(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION
  
  return res;
}

JNIEXPORT jchar JNICALL Java_java_lang_reflect_Field_getChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {
  
  jchar res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      res = (uint16)field->getInt16Field(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
  
  return res;
  
}

JNIEXPORT jshort JNICALL Java_java_lang_reflect_Field_getShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {


  jshort res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      res = (sint16)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isByte())
      res = (sint16)field->getInt8Field(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }
  
  END_NATIVE_EXCEPTION

  return res;
}
  
JNIEXPORT jdouble JNICALL Java_java_lang_reflect_Field_getDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {
  
  jdouble res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      res = (jdouble)(sint64)field->getInt8Field(stat ? StatPtr : obj);
    else if (prim->isInt())
      res = (jdouble)(sint64)field->getInt32Field(stat ? StatPtr : obj);
    else if (prim->isShort())
      res = (jdouble)(sint64)field->getInt16Field(stat? StatPtr : obj);
    else if (prim->isLong())
      res = (jdouble)(sint64)field->getLongField(stat? StatPtr : obj);
    else if (prim->isChar())
      res = (jdouble)(uint64)field->getInt16Field(stat ? StatPtr : obj);
    else if (prim->isFloat())
      res = (jdouble)field->getFloatField(stat ? StatPtr : obj);
    else if (prim->isDouble())
      res = (jdouble)field->getDoubleField(stat ? StatPtr : obj);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Field_get(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj) {


  JavaObject* res = 0;
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool()) {
      uint8 val = field->getInt8Field(stat ? StatPtr : obj);
      res = vm->upcalls->boolClass->doNew(vm);
      vm->upcalls->boolValue->setInt8Field(res, val);
    }
    else if (prim->isByte()) {
      sint8 val =  field->getInt8Field(stat ? StatPtr : obj);
      res = vm->upcalls->byteClass->doNew(vm);
      vm->upcalls->byteValue->setInt8Field(res, val);
    }
    else if (prim->isChar()) {
      uint16 val =  field->getInt16Field(stat ? StatPtr : obj);
      res = vm->upcalls->charClass->doNew(vm);
      vm->upcalls->charValue->setInt16Field(res, val);
    }
    else if (prim->isShort()) {
      sint16 val =  field->getInt16Field(stat ? StatPtr : obj);
      res = vm->upcalls->shortClass->doNew(vm);
      vm->upcalls->shortValue->setInt16Field(res, val);
    }
    else if (prim->isInt()) {
      sint32 val =  field->getInt32Field(stat ? StatPtr : obj);
      res = vm->upcalls->intClass->doNew(vm);
      vm->upcalls->intValue->setInt32Field(res, val);
    }
    else if (prim->isLong()) {
      sint64 val =  field->getLongField(stat ? StatPtr : obj);
      res = vm->upcalls->longClass->doNew(vm);
      vm->upcalls->longValue->setLongField(res, val);
    }
    else if (prim->isFloat()) {
      float val =  field->getFloatField(stat ? StatPtr : obj);
      res = vm->upcalls->floatClass->doNew(vm);
      vm->upcalls->floatValue->setFloatField(res, val);
    }
    else if (prim->isDouble()) {
      double val =  field->getDoubleField(stat ? StatPtr : obj);
      res = vm->upcalls->doubleClass->doNew(vm);
      vm->upcalls->doubleValue->setDoubleField(res, val);
    }
  } else {
    res =  field->getObjectField(stat ? StatPtr : obj);
  }
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, JavaObject* val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(val, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  jvalue buf;
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
 
  const Typedef* type = field->getSignature();
  val->decapsulePrimitive(vm, &buf, type);
  

  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      field->setInt8Field(stat ? StatPtr : obj, buf.z);
    else if (prim->isByte())
      field->setInt8Field(stat ? StatPtr : obj, buf.b);
    else if (prim->isChar())
      field->setInt16Field(stat ? StatPtr : obj, buf.c);
    else if (prim->isShort())
      field->setInt16Field(stat ? StatPtr : obj, buf.s);
    else if (prim->isInt())
      field->setInt32Field(stat ? StatPtr : obj, buf.i);
    else if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, buf.j);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, buf.f);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, buf.d);
  } else {
    field->setObjectField(stat ? StatPtr : obj, val);
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jboolean val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
 
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isBool())
      field->setInt8Field(stat ? StatPtr : obj, (uint8)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
  
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jbyte val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isByte())
      field->setInt8Field(stat ? StatPtr : obj, (sint8)val);
    else if (prim->isShort())
      field->setInt16Field(stat ? StatPtr : obj, (sint16)val);
    else if (prim->isInt())
      field->setInt32Field(stat ? StatPtr : obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jchar val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isChar())
      field->setInt16Field(stat ? StatPtr : obj, (uint16)val);
    else if (prim->isInt())
      field->setInt32Field(stat ? StatPtr : obj, (uint32)val);
    else if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, (uint64)val);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)(uint32)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)(uint64)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jshort val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isShort())
      field->setInt16Field(stat ? StatPtr : obj, (sint16)val);
    else if (prim->isInt())
      field->setInt32Field(stat ? StatPtr : obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jint val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isInt())
      field->setInt32Field(stat ? StatPtr : obj, (sint32)val);
    else if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jlong val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }

  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isLong())
      field->setLongField(stat ? StatPtr : obj, (sint64)val);
    else if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jfloat val) {

  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isFloat())
      field->setFloatField(stat ? StatPtr : obj, (float)val);
    else if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else 
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectField* Field, JavaObject* obj, jdouble val) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = Field->getClass();
  JavaField* field = Field->getInternalField();
  bool stat = isStatic(field->access);
  void* StatPtr = 0;
  
  if (stat) {
    cl->initialiseClass(vm);
    StatPtr = cl->getStaticInstance();
  } else {
    verifyNull(obj);
  }
  
  const Typedef* type = field->getSignature();
  if (type->isPrimitive()) {
    const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
    if (prim->isDouble())
      field->setDoubleField(stat ? StatPtr : obj, (double)val);
    else
      vm->illegalArgumentException("wrong type");
  } else {
    vm->illegalArgumentException("wrong type");
  }

  END_NATIVE_EXCEPTION
}

// Never throws.
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_objectFieldOffset(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* Unsafe, JavaObjectField* Field) {
  
  llvm_gcroot(Field, 0);
  llvm_gcroot(Unsafe, 0);

  JavaField* field = Field->getInternalField();
  return (jlong)field->ptrOffset;
}

}
