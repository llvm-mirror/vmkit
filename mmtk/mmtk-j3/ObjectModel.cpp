//===---- ObjectModel.cpp - Implementation of the ObjectModel class  ------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_getArrayBaseOffset__ (JavaObject* OM) {
  return sizeof(JavaObject) + sizeof(ssize_t);
}

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_GC_1HEADER_1OFFSET__ (JavaObject* OM) {
  return sizeof(void*);
}

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_readAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj->header;
}

extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2 (
    JavaObject* OM, JavaObject* obj, uintptr_t val) {
  obj->header = val;
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_objectStartRef__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_refToAddress__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" uint8_t Java_org_j3_mmtk_ObjectModel_readAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
#if defined(__PPC__)
  return ((uint8_t*)obj)[7];
#else
  return ((uint8_t*)obj)[4];
#endif
}

extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2B (JavaObject* OM, JavaObject* obj, uint8_t val) {
#if defined(__PPC__)
  ((uint8_t*)obj)[7] = val;
#else
  ((uint8_t*)obj)[4] = val;
#endif
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_getObjectFromStartAddress__Lorg_vmmagic_unboxed_Address_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_prepareAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj->header;
}

extern "C" uint8_t
Java_org_j3_mmtk_ObjectModel_attemptAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Word_2(
    JavaObject* OM, JavaObject* obj, intptr_t oldValue, intptr_t newValue) { 
  intptr_t val = __sync_val_compare_and_swap(&(obj->header), oldValue, newValue);
  return (val == oldValue);
}

extern "C" void Java_org_j3_bindings_Bindings_memcpy__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2I(
    void* res, void* src, int size) ALWAYS_INLINE;

extern "C" void Java_org_j3_bindings_Bindings_memcpy__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2I(
    void* res, void* src, int size) {
  memcpy(res, src, size);
}

extern "C" uintptr_t JnJVM_org_j3_bindings_Bindings_copy__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(
    JavaObject* obj, VirtualTable* VT, int size, int allocator) ALWAYS_INLINE;

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_copy__Lorg_vmmagic_unboxed_ObjectReference_2I (
    JavaObject* OM, JavaObject* src, int allocator) ALWAYS_INLINE;

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_copy__Lorg_vmmagic_unboxed_ObjectReference_2I (
    JavaObject* OM, JavaObject* src, int allocator) {
  size_t size = 0;
  VirtualTable* VT = src->getVirtualTable();
  if (VMClassLoader::isVMClassLoader(src)) {
    size = sizeof(VMClassLoader);
  } else {
    CommonClass* cl = JavaObject::getClass(src);
    if (cl->isArray()) {
      UserClassArray* array = cl->asArrayClass();
      UserCommonClass* base = array->baseClass();
      uint32 logSize = base->isPrimitive() ? 
        base->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2); 

      size = sizeof(JavaObject) + sizeof(ssize_t) + 
                    (JavaArray::getSize(src) << logSize);
    } else {
      assert(cl->isClass() && "Not a class!");
      size = cl->asClass()->getVirtualSize();
    }
  }
  size = llvm::RoundUpToAlignment(size, sizeof(void*));
  uintptr_t res = JnJVM_org_j3_bindings_Bindings_copy__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(src, VT, size, allocator);
  assert((((uintptr_t*)res)[1] & ~mvm::GCBitMask) == (((uintptr_t*)src)[1] & ~mvm::GCBitMask));
  return res;
}

extern "C" void Java_org_j3_mmtk_ObjectModel_copyTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 (
    JavaObject* OM, uintptr_t from, uintptr_t to, uintptr_t region) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getReferenceWhenCopiedTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 (
    JavaObject* OM, uintptr_t from, uintptr_t to) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getObjectEndAddress__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getSizeWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignOffsetWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getCurrentSize__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_getNextObject__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }


class FakeByteArray {
 public:
  void* operator new(size_t size, int length) {
    return new char[size + length];
  }

  FakeByteArray(const char* name) {
    length = strlen(name);
    for (uint32 i = 0; i < length; i++) {
      elements[i] = name[i];
    }
  }
  
  FakeByteArray(const UTF8* name) {
    length = name->size;
    for (uint32 i = 0; i < length; i++) {
      elements[i] = name->elements[i];
    }
  }
 private:
  JavaObject header;
  size_t length;
  uint8_t elements[1];
};

extern "C" FakeByteArray* Java_org_j3_mmtk_ObjectModel_getTypeDescriptor__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, JavaObject* src) {
  if (VMClassLoader::isVMClassLoader(src)) {
    return new (14) FakeByteArray("VMClassLoader");
  } else {
    CommonClass* cl = JavaObject::getClass(src);
    return new (cl->name->size) FakeByteArray(cl->name);
  }
}


extern "C" void Java_org_j3_mmtk_ObjectModel_getArrayLength__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_isArray__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_isPrimitiveArray__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_isAcyclic__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_ObjectModel_dumpObject__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* OM, uintptr_t object) { UNIMPLEMENTED(); }
