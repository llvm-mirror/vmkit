//===----------- MvmGC.cpp - Garbage Collection Interface -----------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "MvmGC.h"
#include "MutatorThread.h"

#include <set>

using namespace mvm;

gc::MMTkAllocType           gc::MMTkGCAllocator = 0;
gc::MMTkPostAllocType       gc::MMTkGCPostAllocator = 0;
gc::MMTkCheckAllocatorType  gc::MMTkCheckAllocator = 0;
gc::MMTkDelayedRootType     gc::MMTkDelayedRoot = 0;
gc::MMTkProcessEdgeType     gc::MMTkProcessEdge = 0;
gc::MMTkProcessRootEdgeType gc::MMTkProcessRootEdge = 0;
gc::MMTkIsLiveType          gc::MMTkIsLive = 0;

gc::MMTkRetainReferentType          gc::MMTkRetainReferent = 0;
gc::MMTkRetainForFinalizeType       gc::MMTkRetainForFinalize = 0;
gc::MMTkGetForwardedReferentType    gc::MMTkGetForwardedReferent = 0;
gc::MMTkGetForwardedReferenceType   gc::MMTkGetForwardedReference = 0;
gc::MMTkGetForwardedFinalizableType gc::MMTkGetForwardedFinalizable = 0;
gc::MMTkCollectType                 gc::MMTkTriggerCollection = 0;

uintptr_t Collector::TraceLocal = 0;

  
static std::set<gc*> Set;
static mvm::SpinLock lock;

extern "C" gc* internalMalloc(uintptr_t Mutator, int32_t sz, int32_t align,
                              int32_t offset, int32_t allocator,
                              int32_t site) {
  
  
  gc* res = (gc*)malloc(sz);
  memset(res, 0, sz);
  
  lock.acquire();
  Set.insert(res);
  lock.release();
  
  return res;
}

extern "C" int internalCheckAllocator(uintptr_t Mutator, int32_t sz,
                                      int32_t align, int32_t alloc) {
  return 0;
}

extern "C" void internalPostMalloc(uintptr_t Mutator, uintptr_t ref,
                                   uintptr_t typeref, int32_t bytes,
                                   int32_t allocator) {
}

void* Collector::begOf(gc* obj) {
  if (gc::MMTkGCAllocator == internalMalloc) {
    lock.acquire();
    std::set<gc*>::iterator I = Set.find(obj);
    std::set<gc*>::iterator E = Set.end();
    lock.release();
    
    if (I != E) return obj;
    return 0;
  } else {
    abort();
  }
}

extern "C" void fakeInit(uintptr_t) {
}

extern "C" void fakeInitInt(uintptr_t, int32_t) {
}

void Collector::initialise() {
  if (!gc::MMTkGCAllocator) {
    gc::MMTkGCAllocator = internalMalloc;
    gc::MMTkGCPostAllocator = internalPostMalloc;
    gc::MMTkCheckAllocator = internalCheckAllocator;
    MutatorThread::MMTkMutatorSize = 0;
    MutatorThread::MMTkCollectorSize = 0;
    MutatorThread::MutatorInit = fakeInit;
    MutatorThread::CollectorInit = fakeInit;
    MutatorThread::MutatorCallDeinit = fakeInit;
    MutatorThread::MutatorCallInit = fakeInitInt;
  }
}

