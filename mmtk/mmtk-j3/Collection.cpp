//===------- Collection.cpp - Implementation of the Collection class  -----===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"

using namespace jnjvm;

extern "C" void JnJVM_org_mmtk_plan_Plan_setCollectionTriggered__();

extern "C" void JnJVM_org_j3_config_Selected_00024Collector_staticCollect__();

extern "C" void JnJVM_org_mmtk_plan_Plan_collectionComplete__();


extern "C" bool Java_org_j3_mmtk_Collection_isEmergencyAllocation__ (JavaObject* C) {
  abort();
}

extern "C" void Java_org_j3_mmtk_Collection_reportAllocationSuccess__ (JavaObject* C) {
  abort();
}


extern "C" void Java_org_j3_mmtk_Collection_triggerCollection__I (JavaObject* C, int why) {
  abort();
  JnJVM_org_mmtk_plan_Plan_setCollectionTriggered__();
  JnJVM_org_j3_config_Selected_00024Collector_staticCollect__();
  JnJVM_org_mmtk_plan_Plan_collectionComplete__();
}

extern "C" int Java_org_j3_mmtk_Collection_rendezvous__I (JavaObject* C, int where) {
  abort();
}

extern "C" int Java_org_j3_mmtk_Collection_maximumCollectionAttempt__ (JavaObject* C) {
  abort();
}

extern "C" void Java_org_j3_mmtk_Collection_prepareCollector__Lorg_mmtk_plan_CollectorContext_2 (JavaObject* C, JavaObject* CC) {
  abort();
}


extern "C" void Java_org_j3_mmtk_Collection_joinCollection__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_reportPhysicalAllocationFailed__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_triggerAsyncCollection__I () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_noThreadsInGC__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_prepareMutator__Lorg_mmtk_plan_MutatorContext_2 () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_activeGCThreads__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_activeGCThreadOrdinal__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Collection_requestMutatorFlush__ () { abort(); }
