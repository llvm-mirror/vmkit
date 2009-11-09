//===-------- Selected.cpp - Implementation of the Selected class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "MutatorThread.h"

#include <set>

using namespace jnjvm;

extern "C" gc* internalMalloc(uintptr_t Mutator, int32_t sz, int32_t align,
                              int32_t offset, int32_t allocator,
                              int32_t site);


extern "C" void* gcmalloc(size_t sz, void* _VT) {
  gc* res = 0;
  VirtualTable* VT = (VirtualTable*)_VT;
  sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
  res = internalMalloc(0, sz, 0, 0, 0, 0);
  res->setVirtualTable(VT);
  return res;
}

extern "C" void* gcmallocUnresolved(size_t sz, VirtualTable* VT) {
  gc* res = (gc*)gcmalloc(sz, VT);
  if (VT->destructor)
    mvm::Thread::get()->MyVM->addFinalizationCandidate(res);
  return res;
}

extern "C" void addFinalizationCandidate(gc* obj) {
  mvm::Thread::get()->MyVM->addFinalizationCandidate(obj);
}

