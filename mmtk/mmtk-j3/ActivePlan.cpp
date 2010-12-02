//===------ ActivePlan.cpp - Implementation of the ActivePlan class  ------===//
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
#include "MutatorThread.h"

namespace mmtk {

extern "C" MMTkObject* Java_org_j3_mmtk_ActivePlan_getNextMutator__(MMTkActivePlan* A) {
  assert(A && "No active plan");
  
  if (A->current == NULL) {
    A->current = (mvm::MutatorThread*)mvm::Thread::get()->vmkit()->rendezvous.oneThread;
  } else if (A->current->next() == mvm::Thread::get()->vmkit()->rendezvous.oneThread) {
    A->current = NULL;
    return NULL;
  } else {
    A->current = (mvm::MutatorThread*)A->current->next();
  }

  if (A->current->MutatorContext == 0) {
    return Java_org_j3_mmtk_ActivePlan_getNextMutator__(A);
  }
  return (MMTkObject*)A->current->MutatorContext;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_resetMutatorIterator__(MMTkActivePlan* A) {
  A->current = NULL;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_collectorCount__ (MMTkActivePlan* A) {
  UNIMPLEMENTED();
}

}
