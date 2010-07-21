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
  mvm::MutatorThread* next;
};

extern "C" JavaObject* Java_org_j3_mmtk_ActivePlan_getNextMutator__ (ActivePlan* A) {
  assert(A && "No active plan");
  
  if (A->next == 0) A->next = (mvm::MutatorThread*)JavaThread::get()->MyVM->mainThread;
  else if (A->next->next() != JavaThread::get()->MyVM->mainThread) {
    A->next = 0;
    return 0;
  }
  else A->next = (mvm::MutatorThread*)A->next->next();

  return (JavaObject*)A->next->MutatorContext;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_resetMutatorIterator__ (ActivePlan* A) {
  A->next = 0;
}

extern "C" void Java_org_j3_mmtk_ActivePlan_collectorCount__ () { UNIMPLEMENTED(); }
