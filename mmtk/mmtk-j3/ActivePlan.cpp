//===------ ActivePlan.cpp - Implementation of the ActivePlan class  ------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/VirtualMachine.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

struct ActivePlan {
  JavaObject obj;
  mvm::MutatorThread* current;
};

extern "C" JavaObject* Java_org_j3_mmtk_ActivePlan_getNextMutator__ (ActivePlan* A) {
  assert(A && "No active plan");
  
  if (A->current == NULL) {
    A->current = (mvm::MutatorThread*)JavaThread::get()->MyVM->mainThread;
  } else if (A->current->next() == JavaThread::get()->MyVM->mainThread) {
    A->current = NULL;
    return NULL;
  } else {
    A->current = (mvm::MutatorThread*)A->current->next();
  }

  return (JavaObject*)A->current->MutatorContext;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_resetMutatorIterator__ (ActivePlan* A) {
  A->current = NULL;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_collectorCount__ (ActivePlan* A) { UNIMPLEMENTED(); }
