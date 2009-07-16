//===---- ObjectModel.cpp - Implementation of the ObjectModel class  ------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"

using namespace jnjvm;

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_getArrayBaseOffset__ () {
  return sizeof(JavaObject) + sizeof(ssize_t);
}

extern "C" intptr_t Java_org_j3_mmtk_ObjectModel_GC_1HEADER_1OFFSET__ () {
  return sizeof(void*);
}

extern "C" uintptr_t Java_org_j3_mmtk_ObjectModel_readAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* OM, JavaObject* obj) {
  return ((uintptr_t*)obj)[1];
}

extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableBitsWord__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2 (JavaObject* OM, JavaObject* obj, uintptr_t val) {
  ((uintptr_t*)obj)[1] = val;
}

extern "C" void Java_org_j3_mmtk_ObjectModel_copy__Lorg_vmmagic_unboxed_ObjectReference_2I () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_copyTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getReferenceWhenCopiedTo__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getObjectEndAddress__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getSizeWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getAlignOffsetWhenCopied__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getCurrentSize__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getNextObject__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getObjectFromStartAddress__Lorg_vmmagic_unboxed_Address_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getTypeDescriptor__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_getArrayLength__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isArray__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isPrimitiveArray__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_attemptAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Word_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_prepareAvailableBits__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_writeAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2B () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_readAvailableByte__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_objectStartRef__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_refToAddress__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_isAcyclic__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_ObjectModel_dumpObject__Lorg_vmmagic_unboxed_ObjectReference_2 () { abort(); }
