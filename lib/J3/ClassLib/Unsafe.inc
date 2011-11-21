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

//===--- Direct address read/write acccessors -----------------------------===//
#define GET_PUT_DIRECT(Type,jtype,shorttype) \
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_put ## Type ## __J ## shorttype( \
JavaObject* unsafe, jlong ptr, jtype val) { \
  BEGIN_NATIVE_EXCEPTION(0) \
  *(jtype*)ptr = val; \
  END_NATIVE_EXCEPTION \
} \
 \
JNIEXPORT jtype JNICALL Java_sun_misc_Unsafe_get ## Type ## __J( \
JavaObject* unsafe, jlong ptr) { \
  jtype res = 0; \
  BEGIN_NATIVE_EXCEPTION(0) \
  res = *(jtype*)ptr; \
  END_NATIVE_EXCEPTION \
  return res; \
}

//===--- Volatile variant of field accessors ------------------------------===//
// Memory barriers after writes to ensure new value is seen elsewhere
#define GET_PUT_OFFSET_VOLATILE(Type,jtype,shorttype) \
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_put ## Type ## Volatile__J ## shorttype( \
JavaObject* unsafe, jlong ptr, jtype val) { \
  BEGIN_NATIVE_EXCEPTION(0) \
  *(volatile jtype*)ptr = val; \
  __sync_synchronize(); \
  END_NATIVE_EXCEPTION \
} \
 \
JNIEXPORT jtype JNICALL Java_sun_misc_Unsafe_get ## Type ## Volatile__J( \
JavaObject* unsafe, jlong ptr) { \
  jtype res = 0; \
  BEGIN_NATIVE_EXCEPTION(0) \
  res = *(volatile jtype*)ptr; \
  END_NATIVE_EXCEPTION \
  return res; \
}

//===--- Volatile variant of direct address accessors ---------------------===//
#define GET_PUT_DIRECT_VOLATILE(Type,jtype,shorttype) \
JNIEXPORT jtype JNICALL Java_sun_misc_Unsafe_get ## Type ## Volatile__Ljava_lang_Object_2J( \
JavaObject* unsafe, JavaObject* base, jlong offset) { \
  jtype res = 0; \
  BEGIN_NATIVE_EXCEPTION(0) \
  volatile jtype* ptr = (volatile jtype*)fieldPtr(base,offset); \
  res = *ptr; \
  END_NATIVE_EXCEPTION \
 \
  return res; \
} \
 \
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_put ## Type ## Volatile__Ljava_lang_Object_2J ## shorttype( \
JavaObject* unsafe, JavaObject* base, jlong offset, jtype val) { \
  BEGIN_NATIVE_EXCEPTION(0) \
  volatile jtype* ptr = (volatile jtype*)fieldPtr(base,offset); \
  *ptr = val; \
  __sync_synchronize(); \
  END_NATIVE_EXCEPTION \
}

//===--- Ordered variant of field accessors -------------------------------===//
#define GET_PUT_FIELD_ORDERED(Type,jtype,shorttype) \
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putOrdered ## Type( \
JavaObject* unsafe, JavaObject* base, jlong offset, jtype val) { \
  BEGIN_NATIVE_EXCEPTION(0) \
  volatile jtype* ptr = (volatile jtype*)fieldPtr(base,offset); \
  *ptr = val; \
  /* No memory barrier */ \
  END_NATIVE_EXCEPTION \
}

#define GET_PUT_ALL(Type,jtype,shorttype) \
  GET_PUT_OFFSET(Type,jtype,shorttype) \
  GET_PUT_DIRECT(Type,jtype,shorttype) \
  GET_PUT_OFFSET_VOLATILE(Type,jtype,shorttype) \
  GET_PUT_DIRECT_VOLATILE(Type,jtype,shorttype)

GET_PUT_ALL(Boolean,jboolean,Z)
GET_PUT_ALL(Byte,jbyte,B)
GET_PUT_ALL(Char,jchar,C)
GET_PUT_ALL(Short,jshort,S)
GET_PUT_ALL(Int,jint,I)
GET_PUT_ALL(Long,jlong,J) // TODO: Long needs special handling!
GET_PUT_ALL(Float,jfloat,F)
GET_PUT_ALL(Double,jdouble,D)

// Ordered:
GET_PUT_FIELD_ORDERED(Int,jint,I)
GET_PUT_FIELD_ORDERED(Long,jlong,J)

//===--- Get/Put of Objects, due to GC needs special handling -------------===//
// JavaObject field accessors:
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObject__Ljava_lang_Object_2JLjava_lang_Object_2(
JavaObject* unsafe, JavaObject* base, jlong offset, JavaObject* value) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(value, 0);

  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  vmkit::Collector::objectReferenceWriteBarrier((gc*)base, (gc**)ptr, (gc*)value);
}


JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_getObject__Ljava_lang_Object_2J(
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
JavaObject* unsafe, JavaObject* base, jlong offset, JavaObject* value) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(value, 0);

  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  vmkit::Collector::objectReferenceWriteBarrier((gc*)base, (gc**)ptr, (gc*)value);
  // Ensure this value is seen.
  __sync_synchronize();
}

JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_getObjectVolatile(
JavaObject* unsafe, JavaObject* base, jlong offset) {
  JavaObject * res = 0;
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  JavaObject* volatile* ptr = (JavaObject* volatile*)fieldPtr(base, offset);
  res = *ptr;
  END_NATIVE_EXCEPTION;

  return res;
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putOrderedObject(
JavaObject* unsafe, JavaObject* base, jlong offset, JavaObject* value) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(value, 0);

  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  vmkit::Collector::objectReferenceWriteBarrier((gc*)base, (gc**)ptr, (gc*)value);
  // No barrier (difference between volatile and ordered)
}


//===--- CompareAndSwap field accessors -----------------------------------===//
// Never throws.
JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(
JavaObject* unsafe, JavaObject* base, jlong offset, jlong expect, jlong update) {

  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  jlong *ptr;
  jlong  value;

  ptr = (jlong*)fieldPtr(base, offset);

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
JavaObject* unsafe, JavaObject* base, jlong offset, jint expect, jint update) {

  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  jint *ptr;

  ptr = (jint *)fieldPtr(base, offset);

  return __sync_bool_compare_and_swap(ptr, expect, update);
}

// Never throws.
JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapObject(
JavaObject* unsafe, JavaObject* base, jlong offset, JavaObject* expect,
JavaObject* update) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(base, 0);
  llvm_gcroot(expect, 0);
  llvm_gcroot(update, 0);

  JavaObject** ptr = (JavaObject**)fieldPtr(base, offset);
  return vmkit::Collector::objectReferenceTryCASBarrier((gc*)base, (gc**)ptr, (gc*)expect, (gc*)update);
}


//===--- Class-related functions ------------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_ensureClassInitialized(
JavaObject* unsafe, JavaObject* clazz) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(clazz, 0);
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();

  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, false);
  assert(cl && cl->isClass());
  cl->asClass()->initialiseClass(vm);

  END_NATIVE_EXCEPTION;
}

JNIEXPORT JavaObject* JNICALL Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BII(
JavaObject* unsafe, JavaString *name, ArrayObject * bytesArr, jint off, jint len) {
  UNIMPLEMENTED();
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

JNIEXPORT JavaObject* JNICALL Java_sun_Unsafe_defineAnonymousClass(
JavaObject* unsafe, ...) {
  UNIMPLEMENTED();
}

//===--- Memory functions -------------------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_setMemory__JJB(
JavaObject* unsafe, long address, long bytes, jbyte value) {
  memset((void*)address, value, bytes);
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_copyMemory__JJJ(
JavaObject* unsafe, jlong src, jlong dst, jlong size) {
  memcpy((void*)dst, (void*)src, size);
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_copyMemory__Ljava_lang_Object_2JLjava_lang_Object_2JJ(
JavaObject* unsafe,
JavaObject* srcBase, jlong srcOffset,
JavaObject* dstBase, jlong dstOffset,
jlong size) {
  BEGIN_NATIVE_EXCEPTION(0)
  uint8_t* src = fieldPtr(srcBase, srcOffset, false /* Don't throw on null base*/ );
  uint8_t* dst = fieldPtr(dstBase, dstOffset, false /* Don't throw on null base*/ );
  memcpy(dst, src, size);
  END_NATIVE_EXCEPTION
}

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_allocateMemory(
JavaObject* unsafe, jlong size) {
  // TODO: Invalid size/OOM/etc handling!
  jlong res = 0;
  BEGIN_NATIVE_EXCEPTION(0)
  res = (jlong)malloc(size);
  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_reallocateMemory(
JavaObject* unsafe, jlong ptr, jlong newsize) {
  // TODO: Invalid size/OOM/etc handling!
  jlong res = 0;
  BEGIN_NATIVE_EXCEPTION(0)
  res = (jlong)realloc((void*)ptr, newsize);
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

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_getAddress(
JavaObject* unsafe, jlong ptr) {
  return (jlong)*(void**)ptr;
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putAddress(
JavaObject* unsafe, jlong ptr, jlong val) {
  *(void**)ptr = (void*)val;
}

//===--- Park/Unpark thread support ---------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_park(
JavaObject* unsafe, jboolean isAbsolute, jlong time) {
  // Nothing, for now.
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(
JavaObject* unsafe, JavaObject* thread) {
  // Nothing, for now.
}

//===--- Monitor support --------------------------------------------------===//
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_tryMonitorEnter(
JavaObject* unsafe, JavaObject * obj) {
  //TODO: Implement me!
  UNIMPLEMENTED();
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_monitorEnter(
JavaObject* unsafe, JavaObject * obj) {
  //TODO: Implement me!
  UNIMPLEMENTED();
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_monitorExit(
JavaObject* unsafe, JavaObject * obj) {
  //TODO: Implement me!
  UNIMPLEMENTED();
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

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_pageSize(
JavaObject* unsafe) {
  return vmkit::System::GetPageSize();
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_addressSize(
JavaObject* unsafe) {
  return vmkit::kWordSize;
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_getLoadAverage(
JavaObject* unsafe, ...) {
  UNIMPLEMENTED();
}

}
