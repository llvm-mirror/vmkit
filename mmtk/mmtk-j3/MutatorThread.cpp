//===--------- MutatorThread.cpp - Thread for GC --------------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "MutatorThread.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;

extern "C" void* MMTkMutatorAllocate(uint32_t size, VirtualTable* VT) {
  void* val = MutatorThread::get()->Allocator.Allocate(size, "MMTk");
  ((void**)val)[0] = VT;
  return val;
}
