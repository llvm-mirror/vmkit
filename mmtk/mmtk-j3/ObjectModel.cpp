//===---- ObjectModel.cpp - Implementation of the ObjectModel class  ------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"
#include "JavaThread.h"

using namespace jnjvm;

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_getArrayBaseOffset__ () {
  return sizeof(JavaObject) + sizeof(ssize_t);
}

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_GC_1HEADER_1OFFSET__ () {
  return sizeof(void*);
}

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_readAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return ((uintptr_t*)obj)[1] & mvm::GCMask;
}

extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2 (JavaObject* OM, JavaObject* obj, uintptr_t val) {
  assert((val & ~mvm::GCMask) == (((uintptr_t*)obj)[1] & ~mvm::GCMask) && "GC bits do not fit");
#if defined(__PPC__)
  ((uint8_t*)obj)[7] &= ~mvm::GCMask;
  ((uint8_t*)obj)[7] |= val;
#else
  ((uint8_t*)obj)[4] &= ~mvm::GCMask;
  ((uint8_t*)obj)[4] |= val;
#endif
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_objectStartRef__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_refToAddress__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" uint8_t Java_org_j3_mmtk_ObjectModel_readAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
#if defined(__PPC__)
  return ((uint8_t*)obj)[7] & mvm::GCMask;
#else
  return ((uint8_t*)obj)[4] & mvm::GCMask;
#endif
}

extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2B (JavaObject* OM, JavaObject* obj, uint8_t val) {
  assert((val & mvm::GCMask) == val && "GC bits do not fit");
#if defined(__PPC__)
  ((uint8_t*)obj)[7] &= ~mvm::GCMask;
  ((uint8_t*)obj)[7] |= val;
#else
  ((uint8_t*)obj)[4] &= ~mvm::GCMask;
  ((uint8_t*)obj)[4] |= val;
#endif
}

extern "C" JavaObject* Java_org_j3_mmtk_ObjectModel_getObjectFromStartAddress__Lorg_vmmagic_unboxed_Address_2 (JavaObject* OM, JavaObject* obj) {
  return obj;
}

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_prepareAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return ((intptr_t*)obj)[1];
}

extern "C" uint8_t
Java_org_j3_mmtk_ObjectModel_attemptAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Word_2(
    JavaObject* OM, JavaObject* obj, intptr_t oldValue, intptr_t newValue) {
  
  assert(((oldValue & ~mvm::GCMask) == (newValue & ~mvm::GCMask)) &&
         "GC bits do not fit");
  return __sync_bool_compare_and_swap(((intptr_t*)obj) + 1, oldValue, newValue);
}

extern "C" void Java_org_j3_mmtk_ObjectModel_copy__Lorg_vmmagic_unboxed_ObjectReference_2I () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_copyTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getReferenceWhenCopiedTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getObjectEndAddress__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getSizeWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignOffsetWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getCurrentSize__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getNextObject__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getTypeDescriptor__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getArrayLength__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isArray__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isPrimitiveArray__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isAcyclic__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_dumpObject__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
