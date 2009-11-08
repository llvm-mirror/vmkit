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

extern "C" gc* internalMalloc(uintptr_t Mutator, int32_t sz, int32_t align,
                              int32_t offset, int32_t allocator,
                              int32_t site);
  

extern "C" void* JnJVM_org_mmtk_plan_marksweep_MSMutator_alloc__IIIII(uintptr_t Mutator, int32_t sz, int32_t align, int32_t offset, int32_t allocator, int32_t site) {
  return internalMalloc(Mutator, sz, align, offset, allocator, site);
}
extern "C" int32_t JnJVM_org_mmtk_plan_MutatorContext_checkAllocator__III(uintptr_t Mutator, int32_t bytes, int32_t align, int32_t allocator) {
  return allocator;
}
extern "C" void JnJVM_org_mmtk_plan_marksweep_MSMutator_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II(uintptr_t Mutator, uintptr_t ref, uintptr_t typeref,
    int32_t bytes, int32_t allocator) {}
