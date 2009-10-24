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

using namespace jnjvm;

extern "C" void Java_org_j3_mmtk_TraceInterface_gcEnabled__ () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_adjustSlotOffset__ZLorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_skipOwnFramesAndDump__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_updateDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_setDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Word_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_setLink__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_updateTime__Lorg_vmmagic_unboxed_Word_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getOID__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getDeathTime__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getLink__Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getBootImageLink__ () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getOID__ () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_setOID__Lorg_vmmagic_unboxed_Word_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getHeaderSize__ () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_TraceInterface_getHeaderEndOffset__ () { JavaThread::get()->printBacktrace(); abort(); }

