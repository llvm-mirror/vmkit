//===-------- Selected.cpp - Implementation of the Selected class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "MutatorThread.h"

using namespace jnjvm;

extern "C" void* JnJVM_org_mmtk_plan_marksweep_MSMutator_alloc__IIIII(uintptr_t Mutator, int32_t sz, int32_t align, int32_t offset, int32_t allocator, int32_t site) __attribute__((always_inline));
extern "C" int32_t JnJVM_org_mmtk_plan_MutatorContext_checkAllocator__III(uintptr_t Mutator, int32_t bytes, int32_t align, int32_t allocator) __attribute__((always_inline));
extern "C" void JnJVM_org_mmtk_plan_marksweep_MSMutator_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(uintptr_t Mutator, uintptr_t ref, uintptr_t typeref,
    int32_t bytes, int32_t allocator) __attribute__((always_inline));

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

extern "C" void* gcmallocUnresolved(uint32_t sz, void* _VT) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  VirtualTable* VT = (VirtualTable*)_VT;
  res = (gc*)gcmalloc(sz, VT);
  if (VT->destructor) addFinalizationCandidate(res);
  return res;
}


