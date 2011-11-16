//===-------- Unsafe.inc - sun.misc.Unsafe implementation -----------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "VMStaticInstance.h"
#include <stdlib.h>

/// fieldPtr - Compute the address of the field specified by the given
/// base/offset pair.  Non-trivial due to our handling of static instances,
/// and this also handles null-checking as required.
///
static inline uint8 *fieldPtr(JavaObject *base, int64_t offset,
  bool throwOnNull = true) {

  // For most uses, a 'null' base should throw an exception.
  if (throwOnNull) verifyNull(base);

  if (base && VMStaticInstance::isVMStaticInstance(base))
    return (uint8*)((VMStaticInstance*)base)->getStaticInstance() + offset;
  else
    return (uint8*)base + offset;
}

extern "C" {

//===--- Base/Offset methods ----------------------------------------------===//

/// staticFieldOffset - Return the offset of a particular static field
/// Only valid to be used with the corresponding staticFieldBase
///
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_staticFieldOffset(
JavaObject* unsafe, JavaObjectField* _field) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(_field, 0);

  jlong res = 0;
  BEGIN_NATIVE_EXCEPTION(0)

  JavaField * field = JavaObjectField::getInternalField(_field);
  assert(field);

  res = field->ptrOffset;

  END_NATIVE_EXCEPTION;

  return res;
}

/// staticFieldBase - Return a JavaObject* representing the static instance.
/// Note that our static instances aren't actually java objects, so we use
/// a placeholder object "VMStaticInstance" that also ensures that
/// the corresponding class doesn't get GC'd underneath it.
///
JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_staticFieldBase(
JavaObject* unsafe, JavaObjectField* _field) {
  JavaObject* res = 0;
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(_field, 0);
  llvm_gcroot(res, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  JavaField * field = JavaObjectField::getInternalField(_field);
  assert(field);
  field->classDef->initialiseClass(JavaThread::get()->getJVM());

  res = VMStaticInstance::allocate(field->classDef);

  END_NATIVE_EXCEPTION;

  return res;
}

/// arrayBaseOffset - Offset from the array object where the actual
/// element data begins.
///
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(
JavaObject* unsafe, JavaObject* clazz) {
  // Array starts at beginning of object
  return 0;
}

/// arrayIndexScale - Indexing scale for the element type in
/// the specified array.  For use with arrayBaseOffset,
/// NthElementPtr = ArrayObject + BaseOffset + N*IndexScale
/// Return '0' if our JVM stores the elements in a way that
/// makes this type of access impossible or unsupported.
///
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_arrayIndexScale(
JavaObject* unsafe, JavaObject* clazz) {
  // For now, just return '0', indicating we don't support this indexing.
  // TODO: Implement this for the array types we /do/ support this way.
  return 0;
}



/// objectFieldOffset - Pointer offset of the specified field
///
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_objectFieldOffset(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* Unsafe, JavaObjectField* Field) {

  llvm_gcroot(Field, 0);
  llvm_gcroot(Unsafe, 0);

  JavaField* field = JavaObjectField::getInternalField(Field);
  return (jlong)field->ptrOffset;
}

//===--- Double-register addressing field accessors -----------------------===//
// See objectFieldOffset, staticFieldOffset, staticFieldBase
// Can also be an array, if/when we support arrayIndexScale/arrayBaseOffset
#define GET_PUT_OFFSET(Type,jtype,shorttype) \
JNIEXPORT jtype JNICALL Java_sun_misc_Unsafe_get ## Type ## __Ljava_lang_Object_2J( \
JavaObject* unsafe, JavaObject* base, jlong offset) { \
  jtype res = 0; \
  BEGIN_NATIVE_EXCEPTION(0) \
  jtype* ptr = (jtype*)fieldPtr(base,offset); \
  res = *ptr; \
  END_NATIVE_EXCEPTION \
  return res; \
} \
 \
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_put ## Type ## __Ljava_lang_Object_2J ## shorttype( \
JavaObject* unsafe, JavaObject* base, jlong offset, jtype val) { \
  BEGIN_NATIVE_EXCEPTION(0) \
  jtype* ptr = (jtype*)fieldPtr(base, offset); \
  *ptr = val; \
  END_NATIVE_EXCEPTION \
}

GET_PUT_OFFSET(Boolean,jboolean,Z)
GET_PUT_OFFSET(Byte,jbyte,B)
GET_PUT_OFFSET(Char,jchar,C)
GET_PUT_OFFSET(Short,jshort,S)
GET_PUT_OFFSET(Int,jint,I)
GET_PUT_OFFSET(Long,jlong,J)
GET_PUT_OFFSET(Float,jfloat,F)
GET_PUT_OFFSET(Double,jdouble,D)

//===--- Get/Put of Objects, due to GC needs special handling -------------===//
// JavaObject field accessors:
// Never throws.
JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_getObjectVolatile(
JavaObject* unsafe, JavaObject* base, jlong offset) {
  JavaObject * res = 0;
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  res = *ptr;
  END_NATIVE_EXCEPTION;

  return res;
}

// Volatile JavaObject field accessors:
// Never throws.
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObjectVolatile(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObject* unsafe, JavaObject* base, jlong offset, JavaObject* value) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(value, 0);

  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  mvm::Collector::objectReferenceWriteBarrier((gc*)base, (gc**)ptr, (gc*)value);
}

//===--- Misc get/put defined, TODO: generalize! --------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLong__JJ(
JavaObject* unsafe, jlong ptr, jlong value) {
  BEGIN_NATIVE_EXCEPTION(0)
  *(jlong*)ptr = value;
  END_NATIVE_EXCEPTION
}

JNIEXPORT jbyte JNICALL Java_sun_misc_Unsafe_getByte__J(
JavaObject* unsafe, jlong ptr) {
  jbyte res = 0;
  BEGIN_NATIVE_EXCEPTION(0)
  res =  *(jbyte*)ptr;
  END_NATIVE_EXCEPTION

  return res;
}


//===--- CompareAndSwap field accessors -----------------------------------===//
// Never throws.
JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(
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

//===--- Class-related functions ------------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_ensureClassInitialized(
JavaObject* unsafe, JavaObject* clazz) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(clazz, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();

  CommonClass * cl = JavaObject::getClass(clazz);
  assert(cl && cl->isClass());
  cl->asClass()->resolveClass();
  cl->asClass()->initialiseClass(vm);

  END_NATIVE_EXCEPTION;
}

JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BIILjava_lang_ClassLoader_2Ljava_security_ProtectionDomain_2(
JavaObject* unsafe, JavaString *name, ArrayObject * bytesArr, jint off, jint len, JavaObject * loader, JavaObject * pd) {
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(name, 0);
  llvm_gcroot(bytesArr, 0);
  llvm_gcroot(loader, 0);
  llvm_gcroot(pd, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = NULL;
  JCL = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);

  jint last = off + len;
  if (last < bytesArr->size) {
    assert(0 && "What exception to throw here?");
  }
  ClassBytes * bytes = new (JCL->allocator, len) ClassBytes(len);
  memcpy(bytes->elements, JavaArray::getElements(bytesArr)+off, len);
  const UTF8* utfName = JavaString::javaToInternal(name, JCL->hashUTF8);
  UserClass *cl = JCL->constructClass(utfName, bytes);

  if (cl) res = (JavaObject*)cl->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION;

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_allocateInstance__Ljava_lang_Class_2(
JavaObject* unsafe, JavaObjectClass * clazz) {
  JavaObject* res = 0;
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();

  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  if (cl->isClass())
    res = cl->asClass()->doNew(vm);

  END_NATIVE_EXCEPTION;

  return res;
}


//===--- Memory functions -------------------------------------------------===//
JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_allocateMemory(
JavaObject* unsafe, jlong size) {
  // TODO: Invalid size/OOM/etc handling!
  jlong res = 0;
  BEGIN_NATIVE_EXCEPTION(0)
  res = (jlong)malloc(size);
  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_freeMemory(
JavaObject* unsafe, jlong ptr) {
  // TODO: Exception handling...
  BEGIN_NATIVE_EXCEPTION(0)
  free((void*)ptr);
  END_NATIVE_EXCEPTION
}


//===--- Misc support functions -------------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_registerNatives(JavaObject*) {
  // Nothing, we define the Unsafe methods with the expected signatures.
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_throwException(
JavaObject* unsafe, JavaObject * obj) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(obj, 0);

  JavaThread::get()->throwException(obj);
}



}
