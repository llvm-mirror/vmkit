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

using namespace mvm;

int Collector::verbose = 0;

extern "C" void* JnJVM_org_mmtk_plan_marksweep_MSMutator_alloc__IIIII(uintptr_t Mutator, int32_t sz, int32_t align, int32_t offset, int32_t allocator, int32_t site) __attribute__((always_inline));
extern "C" int32_t JnJVM_org_mmtk_plan_MutatorContext_checkAllocator__III(uintptr_t Mutator, int32_t bytes, int32_t align, int32_t allocator) __attribute__((always_inline));
extern "C" void JnJVM_org_mmtk_plan_marksweep_MSMutator_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(uintptr_t Mutator, uintptr_t ref, uintptr_t typeref,
    int32_t bytes, int32_t allocator) __attribute__((always_inline));

extern "C" uint32_t MMTkMutatorSize;
extern "C" void JnJVM_org_j3_config_Selected_00024Mutator__0003Cinit_0003E__(uintptr_t);
extern "C" VirtualTable* org_j3_config_Selected_4Mutator_VT;
extern "C" void JnJVM_org_mmtk_plan_MutatorContext_initMutator__I(uintptr_t, int32_t);
extern "C" void JnJVM_org_mmtk_plan_MutatorContext_deinitMutator__(uintptr_t);
  
extern "C" void JnJVM_org_j3_config_Selected_00024Collector__0003Cinit_0003E__(uintptr_t);
extern "C" VirtualTable* org_j3_config_Selected_4Collector_VT;
extern "C" uint32_t MMTkCollectorSize;

extern "C" void JnJVM_org_mmtk_utility_heap_HeapGrowthManager_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2(intptr_t, intptr_t);
extern "C" uintptr_t* org_j3_config_Selected_4Plan_static;
extern "C" void MMTkPlanBoot(uintptr_t);
extern "C" void MMTkPlanPostBoot(uintptr_t);
extern "C" void MMTkPlanFullBoot(uintptr_t);
  
extern "C" void Java_org_j3_mmtk_Collection_triggerCollection__I(uintptr_t, int32_t);
//===-------------------- TODO: make those virtual. -------------------===//
extern "C" void JnJVM_org_mmtk_plan_TraceLocal_reportDelayedRootEdge__Lorg_vmmagic_unboxed_Address_2(uintptr_t TraceLocal, void** slot);
extern "C" void JnJVM_org_mmtk_plan_TraceLocal_processEdge__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(
    uintptr_t TraceLocal, void* source, void* slot);
extern "C" void JnJVM_org_mmtk_plan_TraceLocal_processRootEdge__Lorg_vmmagic_unboxed_Address_2Z(
    uintptr_t TraceLocal, void* slot, uint8_t untraced);

extern "C" gc* JnJVM_org_mmtk_plan_TraceLocal_retainForFinalize__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);
extern "C" gc* JnJVM_org_mmtk_plan_TraceLocal_retainReferent__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);
extern "C" gc* JnJVM_org_mmtk_plan_TraceLocal_getForwardedReference__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);
extern "C" gc* JnJVM_org_mmtk_plan_TraceLocal_getForwardedReferent__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);
extern "C" gc* JnJVM_org_mmtk_plan_TraceLocal_getForwardedFinalizable__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);
  
extern "C" uint8_t JnJVM_org_mmtk_plan_marksweep_MSTraceLocal_isLive__Lorg_vmmagic_unboxed_ObjectReference_2(uintptr_t TraceLocal, void* obj);


extern "C" void* gcmalloc(uint32_t sz, void* _VT) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  VirtualTable* VT = (VirtualTable*)_VT;
  sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
  uintptr_t Mutator = mvm::MutatorThread::get()->MutatorContext;
  int allocator = JnJVM_org_mmtk_plan_MutatorContext_checkAllocator__III(Mutator, sz, 0, 0);
  res = (gc*)JnJVM_org_mmtk_plan_marksweep_MSMutator_alloc__IIIII(Mutator, sz, 0, 0, allocator, 0);
  res->setVirtualTable(VT);
  JnJVM_org_mmtk_plan_marksweep_MSMutator_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(Mutator, (uintptr_t)res, (uintptr_t)VT, sz, allocator);
  return res;
}

extern "C" void addFinalizationCandidate(void* obj) __attribute__((always_inline));

extern "C" void addFinalizationCandidate(void* obj) {
  llvm_gcroot(obj, 0);
  mvm::Thread::get()->MyVM->addFinalizationCandidate((gc*)obj);
}

extern "C" void* gcmallocUnresolved(uint32_t sz, VirtualTable* VT) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  res = (gc*)gcmalloc(sz, VT);
  if (VT->destructor) addFinalizationCandidate(res);
  return res;
}

void MutatorThread::init(Thread* _th) {
  MutatorThread* th = (MutatorThread*)_th;
  th->MutatorContext =
    (uintptr_t)th->Allocator.Allocate(MMTkMutatorSize, "Mutator");
  ((VirtualTable**)th->MutatorContext)[0] = org_j3_config_Selected_4Mutator_VT;
  JnJVM_org_j3_config_Selected_00024Mutator__0003Cinit_0003E__(th->MutatorContext);
  JnJVM_org_mmtk_plan_MutatorContext_initMutator__I(th->MutatorContext, (int32_t)_th->getThreadID());
  th->realRoutine(_th);
  JnJVM_org_mmtk_plan_MutatorContext_deinitMutator__(th->MutatorContext);
}

bool Collector::isLive(gc* ptr, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_marksweep_MSTraceLocal_isLive__Lorg_vmmagic_unboxed_ObjectReference_2(closure, ptr);
}

void Collector::scanObject(void** ptr, uintptr_t closure) {
  JnJVM_org_mmtk_plan_TraceLocal_reportDelayedRootEdge__Lorg_vmmagic_unboxed_Address_2(closure, ptr);
}
 
void Collector::markAndTrace(void* source, void* ptr, uintptr_t closure) {
  JnJVM_org_mmtk_plan_TraceLocal_processEdge__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(closure, source, ptr);
}
  
void Collector::markAndTraceRoot(void* ptr, uintptr_t closure) {
  JnJVM_org_mmtk_plan_TraceLocal_processRootEdge__Lorg_vmmagic_unboxed_Address_2Z(closure, ptr, true);
}

gc* Collector::retainForFinalize(gc* val, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_TraceLocal_retainForFinalize__Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::retainReferent(gc* val, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_TraceLocal_retainReferent__Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedFinalizable(gc* val, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_TraceLocal_getForwardedFinalizable__Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReference(gc* val, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_TraceLocal_getForwardedReference__Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReferent(gc* val, uintptr_t closure) {
  return JnJVM_org_mmtk_plan_TraceLocal_getForwardedReferent__Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
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
  void* baseAddr = mmap((void*)0x30000000, 0x40000000, PROT_READ | PROT_WRITE,
                        flags, -1, 0);
  if (baseAddr == MAP_FAILED) {
    perror("mmap");
    abort();
  }
  
  JnJVM_org_mmtk_utility_heap_HeapGrowthManager_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2(128 * 1024 * 1024, 1024 * 1024 * 1024);
  
  uintptr_t Plan = *org_j3_config_Selected_4Plan_static;
  MMTkPlanBoot(Plan);
  MMTkPlanPostBoot(Plan);  
  MMTkPlanFullBoot(Plan);  
}

extern "C" void* MMTkMutatorAllocate(uint32_t size, VirtualTable* VT) {
  void* val = MutatorThread::get()->Allocator.Allocate(size, "MMTk");
  ((void**)val)[0] = VT;
  return val;
}

//TODO: Remove these.
std::set<gc*> __InternalSet__;
void* Collector::begOf(gc* obj) {
  abort();
}
