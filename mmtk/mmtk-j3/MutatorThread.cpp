//===--------- MutatorThread.cpp - Thread for GC --------------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "MutatorThread.h"
#include "MvmGC.h"

using namespace mvm;

uint32_t MutatorThread::MMTkMutatorSize = 0;
uint32_t MutatorThread::MMTkCollectorSize = 0;

void (*MutatorThread::MutatorInit)(uintptr_t) = 0;
void (*MutatorThread::CollectorInit)(uintptr_t) = 0;

gc* (*gc::MMTkGCAllocator)(uintptr_t Mutator, uint32_t sz, uint32_t align,
                           uint32_t offset, uint32_t allocator,
                           uint32_t site);

extern "C" void* MMTkMutatorAllocate(uint32_t size, VirtualTable* VT) {
  void* val = MutatorThread::get()->Allocator.Allocate(size, "MMTk");
  ((void**)val)[0] = VT;
  return val;
}
