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

#define gc_allocator std::allocator
#define gc_new(Class)  __gc_new(Class::VT) Class
#define __gc_new new


#define TRACER tracer()
#define MARK_AND_TRACE markAndTrace()
#define CALL_TRACER tracer()



class gc : public gcRoot {
public:

  void markAndTrace() const {
    fprintf(stderr, "Implement mark and trace\n");
    abort();
  }

  size_t objectSize() const {
    fprintf(stderr, "Implement object size\n");
    abort();
    return 0;
  }

  typedef gc* (*MMTkAllocType)(uintptr_t Mutator, uint32_t sz, uint32_t align,
                               uint32_t offset, uint32_t allocator,
                               uint32_t site);
  
  typedef gc* (*MMTkPostAllocType)(uintptr_t Mutator, uintptr_t ref,
                                   uintptr_t typeref, uint32_t bytes,
                                   uint32_t allocator);

  static MMTkAllocType MMTkGCAllocator;
  
  static MMTkPostAllocType MMTkGCPostAllocator;


  void* operator new(size_t sz, VirtualTable *VT) {
    gc* res = (gc*)MMTkGCAllocator(mvm::MutatorThread::get()->MutatorContext,
                                   sz, 0, 0, 0, 0);
    res->setVirtualTable(VT);
    
    if (VT->destructor) {
      mvm::Thread::get()->MyVM->addFinalizationCandidate(res);
    }
    return res;
  }

};

namespace mvm {
  
class Collector {
public:

  static bool isLive(gc*) {
    abort();
  }
  
  static void traceStackThread() {
    abort();
  }
  
  static void scanObject(void*) {
    abort();
  }
  
  static void collect() {
    abort();
  }
  
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
