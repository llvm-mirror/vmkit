//===- FinalizableProcessor.cpp -------------------------------------------===//
//===- Implementation of the FinalizableProcessor class  ------------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"
#include "mvm/VirtualMachine.h"
#include "mvm/VMKit.h"
#include "MMTkObject.h"

namespace mmtk {

extern "C" void Java_org_j3_mmtk_FinalizableProcessor_clear__ (MMTkObject* P) {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_FinalizableProcessor_forward__Lorg_mmtk_plan_TraceLocal_2Z (MMTkObject* P, uintptr_t TL, uint8_t nursery) {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_FinalizableProcessor_scan__Lorg_mmtk_plan_TraceLocal_2Z (MMTkObject* FP, MMTkObject* TL, uint8_t nursery) {
  mvm::Thread* th = mvm::Thread::get();
  th->vmkit->scanFinalizationQueue(reinterpret_cast<uintptr_t>(TL));
}

}
