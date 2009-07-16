//===----- Statistics.cpp - Implementation of the Statistics class  -------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"

using namespace jnjvm;

extern "C" void Java_org_j3_mmtk_Statistics_perfCtrInit__I (JavaObject* S) {
  // Implement me
}

extern "C" int64_t Java_org_j3_mmtk_Statistics_cycles__ () {
  return 0;
}

extern "C" void Java_org_j3_mmtk_Statistics_getCollectionCount__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Statistics_nanoTime__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Statistics_perfCtrReadCycles__ () { abort(); }
extern "C" void Java_org_j3_mmtk_Statistics_perfCtrReadMetric__ () { abort(); }
