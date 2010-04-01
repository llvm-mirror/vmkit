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

#include "MutatorThread.h"
#include "mvm/VirtualMachine.h"
#include "mvm/GC/GC.h"

#include "llvm/Support/MathExtras.h"


#define gc_allocator std::allocator
#define gc_new(Class)  __gc_new(Class::VT) Class
#define __gc_new new


#define TRACER tracer()
#define MARK_AND_TRACE markAndTrace()
#define CALL_TRACER tracer()


extern "C" void* gcmallocUnresolved(uint32_t sz, VirtualTable* VT);

class gc : public gcRoot {
public:

  size_t objectSize() const {
    fprintf(stderr, "Implement object size\n");
    abort();
    return 0;
  }

  void* operator new(size_t sz, VirtualTable *VT) {
    return gcmallocUnresolved(sz, VT);
  }

};

namespace mvm {
  
class Collector {
public:

  static uintptr_t TraceLocal;

  static bool isLive(gc* ptr);
  
  static void scanObject(void** ptr);
 
  static void markAndTrace(void* source, void* ptr);
  
  static void markAndTraceRoot(void* ptr);

  static gc* retainForFinalize(gc* val);
  
  static gc* retainReferent(gc* val);
  
  static gc* getForwardedFinalizable(gc* val);
  
  static gc* getForwardedReference(gc* val);
  
  static gc* getForwardedReferent(gc* val);

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
