//===- FinalizableProcessor.cpp -------------------------------------------===//
//===- Implementation of the FinalizableProcessor class  ------------------===//
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

extern "C" void Java_org_j3_mmtk_FinalizableProcessor_clear__ () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void
Java_org_j3_mmtk_FinalizableProcessor_forward__Lorg_mmtk_plan_TraceLocal_2Z () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void
Java_org_j3_mmtk_FinalizableProcessor_scan__Lorg_mmtk_plan_TraceLocal_2Z () {
  JavaThread::get()->printBacktrace(); abort();
}
