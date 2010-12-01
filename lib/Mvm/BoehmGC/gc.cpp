//===------------ gc.cc - Boehm GC Garbage Collector ----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;

void Collector::inject_my_thread(mvm::Thread* th) {
  GC_init();
}

void Collector::initialise() {
  GC_INIT();
}

extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT) {
  return (gc*)gc::operator new(sz, VT);
}
