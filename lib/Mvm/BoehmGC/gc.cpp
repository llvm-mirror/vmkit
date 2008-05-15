//===------------ gc.cc - Boehm GC Garbage Collector ----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MvmGC.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;

void Collector::inject_my_thread(void* base_sp) {
  mvm::Thread::get()->baseSP = base_sp;
  GC_init();
}

void Collector::initialise(markerFn mark, void *base_sp) {
  mvm::Thread::get()->baseSP = base_sp;
  GC_INIT();
}

extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT) {
  return (gc*)gc::operator new(sz, VT);
}
