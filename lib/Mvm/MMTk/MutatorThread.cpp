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

MutatorThread::MMTkInitType    MutatorThread::MutatorInit = 0;
MutatorThread::MMTkInitIntType MutatorThread::MutatorCallInit = 0;
MutatorThread::MMTkInitType    MutatorThread::MutatorCallDeinit = 0;
MutatorThread::MMTkInitType    MutatorThread::CollectorInit = 0;

VirtualTable* MutatorThread::MutatorVT = 0;
VirtualTable* MutatorThread::CollectorVT = 0;

extern "C" void* MMTkMutatorAllocate(uint32_t size, VirtualTable* VT) {
  void* val = MutatorThread::get()->Allocator.Allocate(size, "MMTk");
  ((void**)val)[0] = VT;
  return val;
}
