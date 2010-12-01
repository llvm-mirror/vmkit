//===----------- MvmGC.h - Garbage Collection Interface -------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_MMTK_GC_H
#define MVM_MMTK_GC_H

#include <cstdlib>

#define gc_allocator std::allocator

namespace mvm {

class GCVirtualTable : public CommonVirtualTable {
public:
  uintptr_t specializedTracers[1];
  
  static uint32_t numberOfBaseFunctions() {
    return 4;
  }

  static uint32_t numberOfSpecializedTracers() {
    return 1;
  }

  GCVirtualTable(uintptr_t d, uintptr_t o, uintptr_t t) : CommonVirtualTable(d, o, t) {}
  GCVirtualTable() {}
};

extern "C" void* gcmallocUnresolved(uint32_t sz, VirtualTable* VT);

class collectable : public gcRoot {
public:

  size_t objectSize() const {
    abort();
    return 0;
  }

  void* operator new(size_t sz, VirtualTable *VT) {
    return gcmallocUnresolved(sz, VT);
  }

};
  
class Collector {
public:
  static int verbose;

  static bool isLive(gc* ptr, uintptr_t closure) __attribute__ ((always_inline)); 
  static void scanObject(void** ptr, uintptr_t closure) __attribute__ ((always_inline));
  static void markAndTrace(void* source, void* ptr, uintptr_t closure) __attribute__ ((always_inline));
  static void markAndTraceRoot(void* ptr, uintptr_t closure) __attribute__ ((always_inline));
  static gc* retainForFinalize(gc* val, uintptr_t closure) __attribute__ ((always_inline));
  static gc* retainReferent(gc* val, uintptr_t closure) __attribute__ ((always_inline));
  static gc* getForwardedFinalizable(gc* val, uintptr_t closure) __attribute__ ((always_inline));
  static gc* getForwardedReference(gc* val, uintptr_t closure) __attribute__ ((always_inline));
  static gc* getForwardedReferent(gc* val, uintptr_t closure) __attribute__ ((always_inline));

  static void collect();
  
  static void initialise();
  
  static int getMaxMemory() {
    return 0;
  }
  
  static int getFreeMemory() {
    return 0;
  }
  
  static int getTotalMemory() {
    return 0;
  }

  void setMaxMemory(size_t sz){
  }

  void setMinMemory(size_t sz){
  }

  static void* begOf(gc*);
};

}
#endif
