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


extern "C" void MarkAndTrace(gc* gc) {
  Collector::markAndTraceRoot(&gc);
}

extern "C" void* gcmalloc(size_t sz, VirtualTable* VT) {
  void* res = Collector::gcmalloc(VT, sz);
  return res;
}

void Collector::scanObject(void** val) {
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
