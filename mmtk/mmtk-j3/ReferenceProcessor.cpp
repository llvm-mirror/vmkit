//===-------- ReferenceProcessor.cpp --------------------------------------===//
//===-------- Implementation of the Selected class  -----------------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/VirtualMachine.h"

#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

struct ReferenceProcessor {
  JavaObject obj;
  JavaObject* semantics;
  uint32_t ordinal;
};

extern "C" void Java_org_j3_mmtk_ReferenceProcessor_scan__Lorg_mmtk_plan_TraceLocal_2Z (ReferenceProcessor* RP, uintptr_t TL, uint8_t nursery) {
  mvm::Thread* th = mvm::Thread::get();
  uint32_t val = RP->ordinal;

  if (val == 0) {
    th->MyVM->scanSoftReferencesQueue(TL);
  } else if (val == 1) {
    th->MyVM->scanWeakReferencesQueue(TL);
  } else {
    assert(val == 2);
    th->MyVM->scanPhantomReferencesQueue(TL);
  }

}

extern "C" void Java_org_j3_mmtk_ReferenceProcessor_forward__Lorg_mmtk_plan_TraceLocal_2Z (ReferenceProcessor* RP, uintptr_t TL, uint8_t nursery) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_mmtk_ReferenceProcessor_clear__ (ReferenceProcessor* RP) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_mmtk_ReferenceProcessor_countWaitingReferences__ (ReferenceProcessor* RP) { UNIMPLEMENTED(); }
