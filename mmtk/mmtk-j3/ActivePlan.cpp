//===------ ActivePlan.cpp - Implementation of the ActivePlan class  ------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"

using namespace jnjvm;

extern "C" JavaObject* Java_org_j3_mmtk_ActivePlan_getNextMutator__ (JavaObject* A) {
  abort();
}

extern "C" void Java_org_j3_mmtk_ActivePlan_resetMutatorIterator__ (JavaObject* A) {
  abort();
}

extern "C" void Java_org_j3_mmtk_ActivePlan_collectorCount__ () { abort(); }
