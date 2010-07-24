//===-- TraceInterface.cpp - Implementation of the TraceInterface class  --===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

extern "C" bool Java_org_j3_mmtk_TraceInterface_gcEnabled__ (JavaObject* TI) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_adjustSlotOffset__ZLorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 (
    JavaObject* TI, bool scalar, uintptr_t src, uintptr_t slot) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_skipOwnFramesAndDump__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t typeRef) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_updateDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t obj) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_setDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2 (
    JavaObject* TI, uintptr_t ref, uintptr_t time) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_setLink__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t ref, uintptr_t link) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_updateTime__Lorg_vmmagic_unboxed_Word_2 (
    JavaObject* TI, uintptr_t obj) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_getOID__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t ref) { UNIMPLEMENTED(); }

extern "C" uintptr_t Java_org_j3_mmtk_TraceInterface_getDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t ref) { UNIMPLEMENTED(); }

extern "C" uintptr_t Java_org_j3_mmtk_TraceInterface_getLink__Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* TI, uintptr_t ref) { UNIMPLEMENTED(); }

extern "C" uintptr_t Java_org_j3_mmtk_TraceInterface_getBootImageLink__ (
    JavaObject* TI) { UNIMPLEMENTED(); }

extern "C" uintptr_t Java_org_j3_mmtk_TraceInterface_getOID__ (
    JavaObject* TI) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_TraceInterface_setOID__Lorg_vmmagic_unboxed_Word_2 (
    JavaObject* TI, uintptr_t oid) { UNIMPLEMENTED(); }

extern "C" sint32 Java_org_j3_mmtk_TraceInterface_getHeaderSize__ (JavaObject* TI) { UNIMPLEMENTED(); }
extern "C" sint32 Java_org_j3_mmtk_TraceInterface_getHeaderEndOffset__ (JavaObject* TI) { UNIMPLEMENTED(); }

