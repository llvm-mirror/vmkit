//===---------------- gc.cc - Mvm Garbage Collector -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "mvm/Threads/Thread.h"
#include "MvmGC.h"

using namespace mvm;


extern "C" void MarkAndTrace(gc* gc, uintptr_t closure) {
  Collector::markAndTraceRoot(&gc, closure);
}

extern "C" void* gcmalloc(size_t sz, VirtualTable* VT) {
  void* res = Collector::gcmalloc(VT, sz);
  return res;
}

extern "C" void* gcmallocUnresolved(size_t sz, VirtualTable* VT) {
  void* res = 0;
  llvm_gcroot(res, 0);
  res = Collector::gcmalloc(VT, sz);
  return res;
}

extern "C" void addFinalizationCandidate(gc* obj) {
  // This is useless with GCmmap2, as the gcmalloc already did it.
}

extern "C" void fieldWriteBarrier(gc* ref, gc** slot, gc* value) {
  *slot = value;
}

extern "C" void arrayWriteBarrier(gc* ref, gc** slot, gc* value) {
  *slot = value;
}

extern "C" void nonHeapWriteBarrier(gc** slot, gc* value) {
  *slot = value;
}

void Collector::scanObject(void** val, uintptr_t closure) {
  void* obj = *val;
  if (obj) {
    GCChunkNode *node = o2node(obj);

#ifdef WITH_LLVM_GCC
    assert(begOf(obj) == obj && "Interior pointer\n");
    assert(node && "No node in precise GC mode");
#endif
  
    if (node && !Collector::isMarked(node)) {
      node->remove();
      node->append(Collector::used_nodes);
      Collector::mark(node);
    }
  }
}
