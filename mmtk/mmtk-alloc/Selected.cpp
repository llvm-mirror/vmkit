//===-------- Selected.cpp - Implementation of the Selected class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MutatorThread.h"
#include "MvmGC.h"

#include "mvm/VirtualMachine.h"

#include <sys/mman.h>
#include <set>

#include "debug.h"

using namespace mvm;

int Collector::verbose = 0;
extern "C" void Java_org_j3_mmtk_Collection_triggerCollection__I(uintptr_t, int32_t) ALWAYS_INLINE;

extern "C" intptr_t JnJVM_org_j3_bindings_Bindings_allocateMutator__I(int32_t) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_freeMutator__Lorg_mmtk_plan_MutatorContext_2(intptr_t) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2(intptr_t, intptr_t) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_processEdge__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(
    uintptr_t closure, void* source, void* slot) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_reportDelayedRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2(
    uintptr_t TraceLocal, void** slot) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_processRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2Z(
    uintptr_t TraceLocal, void* slot, uint8_t untraced) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_retainForFinalize__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_retainReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedReference__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedFinalizable__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_isLive__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    uintptr_t TraceLocal, void* obj) ALWAYS_INLINE;
  
extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_writeBarrierCAS__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** slot, gc* old, gc* value) ALWAYS_INLINE;
  
extern "C" void JnJVM_org_j3_bindings_Bindings_arrayWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** ptr, gc* value) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_fieldWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** ptr, gc* value) ALWAYS_INLINE;
  
extern "C" void JnJVM_org_j3_bindings_Bindings_nonHeapWriteBarrier__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc** ptr, gc* value) ALWAYS_INLINE;

extern "C" void* JnJVM_org_j3_bindings_Bindings_gcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(
    int sz, void* VT) ALWAYS_INLINE;

extern "C" void* gcmalloc(uint32_t sz, void* VT) {
  sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
  return (gc*)JnJVM_org_j3_bindings_Bindings_gcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(sz, VT);
}

extern "C" void addFinalizationCandidate(gc* obj) __attribute__((always_inline));

extern "C" void addFinalizationCandidate(gc* obj) {
  llvm_gcroot(obj, 0);
  mvm::Thread::get()->MyVM->addFinalizationCandidate(obj);
}

extern "C" void* gcmallocUnresolved(uint32_t sz, VirtualTable* VT) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  res = (gc*)gcmalloc(sz, VT);
  if (VT->destructor) addFinalizationCandidate(res);
  return res;
}

extern "C" void arrayWriteBarrier(void* ref, void** ptr, void* value) {
  JnJVM_org_j3_bindings_Bindings_arrayWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(
      (gc*)ref, (gc**)ptr, (gc*)value);
  if (mvm::Thread::get()->doYield) mvm::Collector::collect();
}

extern "C" void fieldWriteBarrier(void* ref, void** ptr, void* value) {
  JnJVM_org_j3_bindings_Bindings_fieldWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(
      (gc*)ref, (gc**)ptr, (gc*)value);
  if (mvm::Thread::get()->doYield) mvm::Collector::collect();
}

extern "C" void nonHeapWriteBarrier(void** ptr, void* value) {
  JnJVM_org_j3_bindings_Bindings_nonHeapWriteBarrier__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2((gc**)ptr, (gc*)value);
  if (mvm::Thread::get()->doYield) mvm::Collector::collect();
}

void MutatorThread::init(Thread* _th) {
  MutatorThread* th = (MutatorThread*)_th;
  th->MutatorContext =
    JnJVM_org_j3_bindings_Bindings_allocateMutator__I((int32_t)_th->getThreadID());
  th->realRoutine(_th);
  uintptr_t context = th->MutatorContext;
  th->MutatorContext = 0;
  JnJVM_org_j3_bindings_Bindings_freeMutator__Lorg_mmtk_plan_MutatorContext_2(context);
}

bool Collector::isLive(gc* ptr, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_isLive__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, ptr);
}

void Collector::scanObject(void** ptr, uintptr_t closure) {
  if ((*ptr) != NULL) {
    assert(((gc*)(*ptr))->getVirtualTable());
  }
  JnJVM_org_j3_bindings_Bindings_reportDelayedRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2(closure, ptr);
}
 
void Collector::markAndTrace(void* source, void* ptr, uintptr_t closure) {
  void** ptr_ = (void**)ptr;
  if ((*ptr_) != NULL) {
    assert(((gc*)(*ptr_))->getVirtualTable());
  }
  if ((*(void**)ptr) != NULL) assert(((gc*)(*(void**)ptr))->getVirtualTable());
  JnJVM_org_j3_bindings_Bindings_processEdge__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(closure, source, ptr);
}
  
void Collector::markAndTraceRoot(void* ptr, uintptr_t closure) {
  void** ptr_ = (void**)ptr;
  if ((*ptr_) != NULL) {
    assert(((gc*)(*ptr_))->getVirtualTable());
  }
  JnJVM_org_j3_bindings_Bindings_processRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2Z(closure, ptr, true);
}

gc* Collector::retainForFinalize(gc* val, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_retainForFinalize__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::retainReferent(gc* val, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_retainReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedFinalizable(gc* val, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_getForwardedFinalizable__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReference(gc* val, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_getForwardedReference__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReferent(gc* val, uintptr_t closure) {
  return JnJVM_org_j3_bindings_Bindings_getForwardedReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}

void Collector::collect() {
  Java_org_j3_mmtk_Collection_triggerCollection__I(NULL, 2);
}

void Collector::initialise() {
  // Allocate the memory for MMTk right now, to avoid conflicts with
  // other allocators.
#if defined (__MACH__)
  uint32 flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
#else
  uint32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
  void* baseAddr = mmap((void*)0x30000000, 0x30000000, PROT_READ | PROT_WRITE,
                        flags, -1, 0);
  if (baseAddr == MAP_FAILED) {
    perror("mmap");
    abort();
  }
  
  JnJVM_org_j3_bindings_Bindings_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2(128 * 1024 * 1024, 1024 * 1024 * 1024);
}

extern "C" void* MMTkMutatorAllocate(uint32_t size, VirtualTable* VT) {
  void* val = MutatorThread::get()->Allocator.Allocate(size);
  ((void**)val)[0] = VT;
  return val;
}

void Collector::objectReferenceWriteBarrier(gc* ref, gc** slot, gc* value) {
  fieldWriteBarrier((void*)ref, (void**)slot, (void*)value);
}

void Collector::objectReferenceArrayWriteBarrier(gc* ref, gc** slot, gc* value) {
  arrayWriteBarrier((void*)ref, (void**)slot, (void*)value);
}

void Collector::objectReferenceNonHeapWriteBarrier(gc** slot, gc* value) {
  nonHeapWriteBarrier((void**)slot, (void*)value);
}

bool Collector::objectReferenceTryCASBarrier(gc* ref, gc** slot, gc* old, gc* value) {
  bool res = JnJVM_org_j3_bindings_Bindings_writeBarrierCAS__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2(ref, slot, old, value);
  if (mvm::Thread::get()->doYield) mvm::Collector::collect();
  return res;
}

extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_needsWriteBarrier__() ALWAYS_INLINE;
extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_needsNonHeapWriteBarrier__() ALWAYS_INLINE;

bool Collector::needsWriteBarrier() {
  return JnJVM_org_j3_bindings_Bindings_needsWriteBarrier__();
}

bool Collector::needsNonHeapWriteBarrier() {
  return JnJVM_org_j3_bindings_Bindings_needsNonHeapWriteBarrier__();
}

//TODO: Remove these.
std::set<gc*> __InternalSet__;
void* Collector::begOf(gc* obj) {
  abort();
}
