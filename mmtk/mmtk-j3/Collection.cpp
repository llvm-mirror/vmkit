//===------- Collection.cpp - Implementation of the Collection class  -----===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"
#include "mvm/VirtualMachine.h"
#include "MMTkObject.h"
#include "MvmGC.h"

namespace mmtk {

extern "C" void JnJVM_org_mmtk_plan_Plan_setCollectionTriggered__();
extern "C" void JnJVM_org_j3_config_Selected_00024Collector_staticCollect__();
extern "C" void JnJVM_org_mmtk_plan_Plan_collectionComplete__();
extern "C" uint8_t JnJVM_org_mmtk_utility_heap_HeapGrowthManager_considerHeapSize__();
extern "C" void JnJVM_org_mmtk_utility_heap_HeapGrowthManager_reset__();
extern "C" int64_t Java_org_j3_mmtk_Statistics_nanoTime__ ();
extern "C" void JnJVM_org_mmtk_utility_heap_HeapGrowthManager_recordGCTime__D(double);

extern "C" bool Java_org_j3_mmtk_Collection_isEmergencyAllocation__ (MMTkObject* C) {
  // TODO: emergency when OOM.
  return false;
}

extern "C" void Java_org_j3_mmtk_Collection_reportAllocationSuccess__ (MMTkObject* C) {
  // TODO: clear internal data.
}

extern "C" void Java_org_j3_mmtk_Collection_triggerCollection__I (MMTkObject* C, int why) {
  mvm::Thread* th = mvm::Thread::get();

  // Verify that another collection is not happening.
  th->MyVM->rendezvous.startRV();
  if (th->MyVM->rendezvous.getInitiator() != NULL) {
    th->MyVM->rendezvous.cancelRV();
    th->MyVM->rendezvous.join();
    return;
  } else {
    th->MyVM->startCollection();
    th->MyVM->rendezvous.synchronize();
  
    JnJVM_org_mmtk_plan_Plan_setCollectionTriggered__();

    // Record the starting time
    int64_t startTime = Java_org_j3_mmtk_Statistics_nanoTime__();

    // Collect!
    JnJVM_org_j3_config_Selected_00024Collector_staticCollect__();

    // Record the time to GC.
    int64_t elapsedTime = Java_org_j3_mmtk_Statistics_nanoTime__() - startTime;

    JnJVM_org_mmtk_utility_heap_HeapGrowthManager_recordGCTime__D(((double)elapsedTime) / 1000000);

    // 2 means called by System.gc();
    if (why != 2)
      JnJVM_org_mmtk_utility_heap_HeapGrowthManager_considerHeapSize__();

    JnJVM_org_mmtk_utility_heap_HeapGrowthManager_reset__();

    JnJVM_org_mmtk_plan_Plan_collectionComplete__();
    
    if (mvm::Collector::verbose > 0) {
      static int collectionNum = 1;
      if (why == 2) fprintf(stderr, "[Forced] ");
      fprintf(stderr, "Collection %d finished in %lld ms.\n", collectionNum++,
              elapsedTime / 1000000);
    }

    th->MyVM->rendezvous.finishRV();
    th->MyVM->endCollection();
  }

}

extern "C" void Java_org_j3_mmtk_Collection_joinCollection__ (MMTkObject* C) {
  mvm::Thread* th = mvm::Thread::get();
  assert(th->inRV && "Joining collection without a rendezvous");
  th->MyVM->rendezvous.join();
}

extern "C" int Java_org_j3_mmtk_Collection_rendezvous__I (MMTkObject* C, int where) {
  return 1;
}

extern "C" int Java_org_j3_mmtk_Collection_maximumCollectionAttempt__ (MMTkObject* C) {
  return 0;
}

extern "C" void Java_org_j3_mmtk_Collection_prepareCollector__Lorg_mmtk_plan_CollectorContext_2 (MMTkObject* C, MMTkObject* CC) {
  // Nothing to do.
}

extern "C" void Java_org_j3_mmtk_Collection_prepareMutator__Lorg_mmtk_plan_MutatorContext_2 (MMTkObject* C, MMTkObject* MC) {
}

extern "C" int32_t Java_org_j3_mmtk_Collection_activeGCThreads__ (MMTkObject* C) { return 1; }
extern "C" int32_t Java_org_j3_mmtk_Collection_activeGCThreadOrdinal__ (MMTkObject* C) { return 1; }


extern "C" void Java_org_j3_mmtk_Collection_reportPhysicalAllocationFailed__ (MMTkObject* C) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_mmtk_Collection_triggerAsyncCollection__I (MMTkObject* C, sint32 val) {
  mvm::Thread::get()->doYield = true;
}

extern "C" uint8_t Java_org_j3_mmtk_Collection_noThreadsInGC__ (MMTkObject* C) {
  mvm::Thread* th = mvm::Thread::get();
  bool threadInGC = th->doYield;
  return !threadInGC;
}

extern "C" void Java_org_j3_mmtk_Collection_requestMutatorFlush__ (MMTkObject* C) { UNIMPLEMENTED(); }

} // namespace mmtk
