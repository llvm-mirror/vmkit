//===-------- ReferenceProcessor.cpp --------------------------------------===//
//===-------- Implementation of the Selected class  -----------------------===//
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

extern "C" void Java_org_j3_mmtk_ReferenceProcessor_scan__Lorg_mmtk_plan_TraceLocal_2Z () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void Java_org_j3_mmtk_ReferenceProcessor_forward__Lorg_mmtk_plan_TraceLocal_2Z () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ReferenceProcessor_clear__ () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_ReferenceProcessor_countWaitingReferences__ () { JavaThread::get()->printBacktrace(); abort(); }
