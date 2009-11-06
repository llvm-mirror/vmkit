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



class gc : public gcRoot {
public:

  size_t objectSize() const {
    fprintf(stderr, "Implement object size\n");
    abort();
    return 0;
  }

  typedef gc* (*MMTkAllocType)(uintptr_t Mutator, int32_t sz, int32_t align,
                               int32_t offset, int32_t allocator,
                               int32_t site);
  
  typedef void (*MMTkPostAllocType)(uintptr_t Mutator, uintptr_t ref,
                                    uintptr_t typeref, int32_t bytes,
                                    int32_t allocator);

  typedef int (*MMTkCheckAllocatorType)(uintptr_t Mutator, int32_t bytes,
                                        int32_t align, int32_t allocator);

  typedef void (*MMTkDelayedRootType)(uintptr_t TraceLocal, void** slot);
  
  typedef void (*MMTkProcessEdgeType)(uintptr_t TraceLocal, void* source,
                                      void* slot);
  
  typedef void (*MMTkProcessRootEdgeType)(uintptr_t TraceLocal, void* slot,
                                          uint8_t untraced);
  
  typedef uint8_t (*MMTkIsLiveType)(uintptr_t TraceLocal, void* obj);

  typedef gc* (*MMTkRetainReferentType)(uintptr_t TraceLocal, void* obj);
  typedef MMTkRetainReferentType MMTkRetainForFinalizeType;
  typedef MMTkRetainReferentType MMTkGetForwardedReferenceType;
  typedef MMTkRetainReferentType MMTkGetForwardedReferentType;
  typedef MMTkRetainReferentType MMTkGetForwardedFinalizableType;
  typedef void (*MMTkCollectType)(uintptr_t, int32_t);

  static MMTkAllocType MMTkGCAllocator;
  
  static MMTkPostAllocType MMTkGCPostAllocator;
  
  static MMTkCheckAllocatorType MMTkCheckAllocator;
  
  static MMTkDelayedRootType MMTkDelayedRoot;
  
  static MMTkProcessEdgeType MMTkProcessEdge;
  
  static MMTkProcessRootEdgeType MMTkProcessRootEdge;
  
  static MMTkIsLiveType MMTkIsLive;
  
  static MMTkRetainReferentType MMTkRetainReferent;
  static MMTkRetainForFinalizeType MMTkRetainForFinalize;
  static MMTkGetForwardedReferenceType MMTkGetForwardedReference;
  static MMTkGetForwardedReferentType MMTkGetForwardedReferent;
  static MMTkGetForwardedFinalizableType MMTkGetForwardedFinalizable;

  static MMTkCollectType MMTkTriggerCollection;

  void* operator new(size_t sz, VirtualTable *VT) {
    gc* res = 0;
    llvm_gcroot(res, 0);
    assert(VT->tracer && "VT without a tracer");
    sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
    uintptr_t Mutator = mvm::MutatorThread::get()->MutatorContext;
    int allocator = MMTkCheckAllocator(Mutator, sz, 0, 0);
    res = (gc*)MMTkGCAllocator(Mutator, sz, 0, 0, allocator, 0);
    assert(res && "Allocation failed");
    assert(res->getVirtualTable() == 0 && "Allocation not zeroed");
    res->setVirtualTable(VT);
    MMTkGCPostAllocator(Mutator, (uintptr_t)res, (uintptr_t)VT, sz, allocator);
    
    if (VT->destructor) {
      mvm::Thread::get()->MyVM->addFinalizationCandidate(res);
    }
    return res;
  }

};

namespace mvm {
  
class Collector {
public:

  static uintptr_t TraceLocal;

  static bool isLive(gc* ptr) {
    return gc::MMTkIsLive(TraceLocal, ptr);
  }
  
  static void scanObject(void** ptr) {
    assert(gc::MMTkDelayedRoot && "scanning without a function");
    assert(TraceLocal && "scanning without a trace local");
    gc::MMTkDelayedRoot(TraceLocal, ptr);
  }
 
  static void markAndTrace(void* source, void* ptr) {
    assert(TraceLocal && "scanning without a trace local");
    gc::MMTkProcessEdge(TraceLocal, source, ptr);
  }
  
  static void markAndTraceRoot(void* ptr) {
    assert(TraceLocal && "scanning without a trace local");
    gc::MMTkProcessRootEdge(TraceLocal, ptr, true);
  }

  static gc* retainForFinalize(gc* val) {
    assert(TraceLocal && "scanning without a trace local");
    return gc::MMTkRetainForFinalize(TraceLocal, val);
  }
  
  static gc* retainReferent(gc* val) {
    assert(TraceLocal && "scanning without a trace local");
    return gc::MMTkRetainReferent(TraceLocal, val);
  }
  
  static gc* getForwardedFinalizable(gc* val) {
    assert(TraceLocal && "scanning without a trace local");
    return gc::MMTkGetForwardedFinalizable(TraceLocal, val);
  }
  
  static gc* getForwardedReference(gc* val) {
    assert(TraceLocal && "scanning without a trace local");
    return gc::MMTkGetForwardedReference(TraceLocal, val);
  }
  
  static gc* getForwardedReferent(gc* val) {
    assert(TraceLocal && "scanning without a trace local");
    return gc::MMTkGetForwardedReferent(TraceLocal, val);
  }

  static void collect() {
    if (gc::MMTkTriggerCollection) gc::MMTkTriggerCollection(NULL, 2);
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
