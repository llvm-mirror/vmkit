//===-------- Selected.cpp - Implementation of the Selected class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MutatorThread.h"
#include "VmkitGC.h"
#include "../mmtk-j3/MMTkObject.h"

#include "vmkit/VirtualMachine.h"

#include <sys/mman.h>
#include <set>

#include "debug.h"

using namespace vmkit;

int Collector::verbose = 0;
extern "C" void Java_org_j3_mmtk_Collection_triggerCollection__I(word_t, int32_t) ALWAYS_INLINE;

extern "C" word_t JnJVM_org_j3_bindings_Bindings_allocateMutator__I(int32_t) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_freeMutator__Lorg_mmtk_plan_MutatorContext_2(word_t) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2_3Ljava_lang_String_2(word_t, word_t, mmtk::MMTkObjectArray*) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_processEdge__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(
    word_t closure, void* source, void* slot) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_reportDelayedRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2(
    word_t TraceLocal, void** slot) ALWAYS_INLINE;
extern "C" void JnJVM_org_j3_bindings_Bindings_processRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2Z(
    word_t TraceLocal, void* slot, uint8_t untraced) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_retainForFinalize__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_retainReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedReference__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" gc* JnJVM_org_j3_bindings_Bindings_getForwardedFinalizable__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_isLive__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(
    word_t TraceLocal, void* obj) ALWAYS_INLINE;
  
extern "C" uint8_t JnJVM_org_j3_bindings_Bindings_writeBarrierCAS__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** slot, gc* old, gc* value) ALWAYS_INLINE;
  
extern "C" void JnJVM_org_j3_bindings_Bindings_arrayWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** ptr, gc* value) ALWAYS_INLINE;

extern "C" void JnJVM_org_j3_bindings_Bindings_fieldWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc* ref, gc** ptr, gc* value) ALWAYS_INLINE;
  
extern "C" void JnJVM_org_j3_bindings_Bindings_nonHeapWriteBarrier__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(gc** ptr, gc* value) ALWAYS_INLINE;

extern "C" void* JnJVM_org_j3_bindings_Bindings_prealloc__I(int sz) ALWAYS_INLINE;

extern "C" void* JnJVM_org_j3_bindings_Bindings_postalloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2I(
		void* object, void* type, int sz) ALWAYS_INLINE;

extern "C" void* JnJVM_org_j3_bindings_Bindings_vmkitgcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(
    int sz, void* VT) ALWAYS_INLINE;

extern "C" void* JnJVM_org_j3_bindings_Bindings_VTgcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(
    int sz, void* VT) ALWAYS_INLINE;

extern "C" void addFinalizationCandidate(gc* obj) ALWAYS_INLINE;

/**************************************
 * Sample of code using pre/post alloc. It is slower but you can have
 * a better control of objects allocation.
 *
extern "C" void* vmkitgcmalloc(uint32_t sz, void* type) {
  gc* res = 0;
  llvm_gcroot(res, 0);
	sz += gcHeader::hiddenHeaderSize();
	res = (gc*) prealloc(sz);
	postalloc(res, type, sz);
	return res;
}
*/
extern "C" void* prealloc(uint32_t size) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  gcHeader* head = 0;
  llvm_gcroot(res, 0);
  size = llvm::RoundUpToAlignment(size, sizeof(void*));
  head = (gcHeader*) JnJVM_org_j3_bindings_Bindings_prealloc__I(size);
  res = head->toReference();
  return res;
}

extern "C" void postalloc(gc* obj, void* type, uint32_t size) {
	llvm_gcroot(obj, 0);
	vmkit::Thread::get()->MyVM->setType(obj, type);
	JnJVM_org_j3_bindings_Bindings_postalloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2I(obj, type, size);
}

extern "C" void* vmkitgcmalloc(uint32_t sz, void* type) {
	gc* res = 0;
	llvm_gcroot(res, 0);
	sz += gcHeader::hiddenHeaderSize();
	sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
	res = ((gcHeader*)JnJVM_org_j3_bindings_Bindings_vmkitgcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(sz, type))->toReference();
	return res;
}

extern "C" void* vmkitgcmallocUnresolved(uint32_t sz, void* type) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  res = (gc*)vmkitgcmalloc(sz, type);
	addFinalizationCandidate(res);
  return res;
}

/******************************************************************************
 * Optimized gcmalloc for VT based object layout.                             *
 *****************************************************************************/

extern "C" void* VTgcmalloc(uint32_t sz, void* VT) {
	gc* res = 0;
	llvm_gcroot(res, 0);
	sz += gcHeader::hiddenHeaderSize();
	sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
	res = ((gcHeader*)JnJVM_org_j3_bindings_Bindings_VTgcmalloc__ILorg_vmmagic_unboxed_ObjectReference_2(sz, VT))->toReference();
	return res;
}

extern "C" void* VTgcmallocUnresolved(uint32_t sz, void* VT) {
  gc* res = 0;
  llvm_gcroot(res, 0);
  res = (gc*)VTgcmalloc(sz, VT);
  if (((VirtualTable*)VT)->hasDestructor()) addFinalizationCandidate(res);
  return res;
}

/*****************************************************************************/

extern "C" void addFinalizationCandidate(gc* obj) {
  llvm_gcroot(obj, 0);
  vmkit::Thread::get()->MyVM->addFinalizationCandidate(obj);
}

extern "C" void arrayWriteBarrier(void* ref, void** ptr, void* value) {
  llvm_gcroot(ref, 0);
  llvm_gcroot(value, 0);
  JnJVM_org_j3_bindings_Bindings_arrayWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(
      (gc*)ref, (gc**)ptr, (gc*)value);
  if (vmkit::Thread::get()->doYield) vmkit::Collector::collect();
}

extern "C" void fieldWriteBarrier(void* ref, void** ptr, void* value) {
  llvm_gcroot(ref, 0);
  llvm_gcroot(value, 0);
  JnJVM_org_j3_bindings_Bindings_fieldWriteBarrier__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2(
      (gc*)ref, (gc**)ptr, (gc*)value);
  if (vmkit::Thread::get()->doYield) vmkit::Collector::collect();
}

extern "C" void nonHeapWriteBarrier(void** ptr, void* value) {
  llvm_gcroot(value, 0);
  JnJVM_org_j3_bindings_Bindings_nonHeapWriteBarrier__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2((gc**)ptr, (gc*)value);
  if (vmkit::Thread::get()->doYield) vmkit::Collector::collect();
}

void MutatorThread::init(Thread* _th) {
  MutatorThread* th = (MutatorThread*)_th;
  th->MutatorContext =
    JnJVM_org_j3_bindings_Bindings_allocateMutator__I((int32_t)_th->getThreadID());
  th->realRoutine(_th);
  word_t context = th->MutatorContext;
  th->MutatorContext = 0;
  JnJVM_org_j3_bindings_Bindings_freeMutator__Lorg_mmtk_plan_MutatorContext_2(context);
}

bool Collector::isLive(gc* ptr, word_t closure) {
  llvm_gcroot(ptr, 0);
  return JnJVM_org_j3_bindings_Bindings_isLive__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, ptr);
}

void Collector::scanObject(FrameInfo* FI, void** ptr, word_t closure) {
  if ((*ptr) != NULL) {
    assert(vmkit::Thread::get()->MyVM->isCorruptedType((gc*)(*ptr)));
  }
#if RESET_STALE_REFERENCES
  // Allow the VM to reset references if needed
  vmkit::Thread::get()->MyVM->resetReferenceIfStale(NULL, ptr);
#endif
  JnJVM_org_j3_bindings_Bindings_reportDelayedRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2(closure, ptr);
}
 
void Collector::markAndTrace(void* source, void* ptr, word_t closure) {
	llvm_gcroot(source, 0);
	void** ptr_ = (void**)ptr;
	if ((*ptr_) != NULL) {
		assert(vmkit::Thread::get()->MyVM->isCorruptedType((gc*)(*ptr_)));
	}
	if ((*(void**)ptr) != NULL) assert(vmkit::Thread::get()->MyVM->isCorruptedType((gc*)(*(void**)ptr)));
#if RESET_STALE_REFERENCES
	// Allow the VM to reset references if needed
	vmkit::Thread::get()->MyVM->resetReferenceIfStale(source, ptr_);
#endif
	JnJVM_org_j3_bindings_Bindings_processEdge__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2(closure, source, ptr);
}
  
void Collector::markAndTraceRoot(void* source, void* ptr, word_t closure) {
  llvm_gcroot(source, 0);
  void** ptr_ = (void**)ptr;
  if ((*ptr_) != NULL) {
    assert(vmkit::Thread::get()->MyVM->isCorruptedType((gc*)(*ptr_)));
  }
#if RESET_STALE_REFERENCES
  // Allow the VM to reset references if needed
  vmkit::Thread::get()->MyVM->resetReferenceIfStale(source, ptr_);
#endif
  JnJVM_org_j3_bindings_Bindings_processRootEdge__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_Address_2Z(closure, ptr, true);
}

gc* Collector::retainForFinalize(gc* val, word_t closure) {
  llvm_gcroot(val, 0);
  return JnJVM_org_j3_bindings_Bindings_retainForFinalize__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::retainReferent(gc* val, word_t closure) {
  llvm_gcroot(val, 0);
  return JnJVM_org_j3_bindings_Bindings_retainReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedFinalizable(gc* val, word_t closure) {
  llvm_gcroot(val, 0);
  return JnJVM_org_j3_bindings_Bindings_getForwardedFinalizable__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReference(gc* val, word_t closure) {
  llvm_gcroot(val, 0);
  return JnJVM_org_j3_bindings_Bindings_getForwardedReference__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}
  
gc* Collector::getForwardedReferent(gc* val, word_t closure) {
  llvm_gcroot(val, 0);
  return JnJVM_org_j3_bindings_Bindings_getForwardedReferent__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2(closure, val);
}

void Collector::collect() {
  Java_org_j3_mmtk_Collection_triggerCollection__I(0, 2);
}
  
static const char* kPrefix = "-X:gc:";
static const int kPrefixLength = strlen(kPrefix);

void Collector::initialise(int argc, char** argv) {
  int i = 1;
  int count = 0;
  ThreadAllocator allocator;
  mmtk::MMTkObjectArray* arguments = NULL;
  while (i < argc && argv[i][0] == '-') {
    if (!strncmp(argv[i], kPrefix, kPrefixLength)) {
      count++;
    }
    i++;
  }

  if (count > 0) {
    arguments = reinterpret_cast<mmtk::MMTkObjectArray*>(
        malloc(sizeof(mmtk::MMTkObjectArray) + count * sizeof(mmtk::MMTkString*)));
    arguments->size = count;
    i = 1;
    int arrayIndex = 0;
    while (i < argc && argv[i][0] == '-') {
      if (!strncmp(argv[i], kPrefix, kPrefixLength)) {
        int size = strlen(argv[i]) - kPrefixLength;
        mmtk::MMTkArray* array = reinterpret_cast<mmtk::MMTkArray*>(
            allocator.Allocate(sizeof(mmtk::MMTkArray) + size * sizeof(uint16_t)));
        array->size = size;
        for (uint32_t j = 0; j < array->size; j++) {
          array->elements[j] = argv[i][j + kPrefixLength];
        }
        mmtk::MMTkString* str = reinterpret_cast<mmtk::MMTkString*>(
            allocator.Allocate(sizeof(mmtk::MMTkString)));
        str->value = array;
        str->count = array->size;
        str->offset = 0;
        arguments->elements[arrayIndex++] = str;
      }
      i++;
    }
    assert(arrayIndex == count);
  }

  JnJVM_org_j3_bindings_Bindings_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2_3Ljava_lang_String_2(20 * 1024 * 1024, 100 * 1024 * 1024, arguments);
}

extern "C" void* MMTkMutatorAllocate(uint32_t size, void* type) {
  gcHeader* head = NULL;
  size += gcHeader::hiddenHeaderSize();
  size = llvm::RoundUpToAlignment(size, sizeof(void*));
  head = (gcHeader*)MutatorThread::get()->Allocator.Allocate(size);
  void* val = head->toReference();
  VirtualTable::setVirtualTable((gc*)val, (VirtualTable*)type);
//  vmkit::Thread::get()->MyVM->setType(val, type);
  return val;
}

void Collector::objectReferenceWriteBarrier(gc* ref, gc** slot, gc* value) {
  llvm_gcroot(ref, 0);
  llvm_gcroot(value, 0);
  fieldWriteBarrier((void*)ref, (void**)slot, (void*)value);
}

void Collector::objectReferenceArrayWriteBarrier(gc* ref, gc** slot, gc* value) {
  llvm_gcroot(ref, 0);
  llvm_gcroot(value, 0);
  arrayWriteBarrier((void*)ref, (void**)slot, (void*)value);
}

void Collector::objectReferenceNonHeapWriteBarrier(gc** slot, gc* value) {
  llvm_gcroot(value, 0);
  nonHeapWriteBarrier((void**)slot, (void*)value);
}

bool Collector::objectReferenceTryCASBarrier(gc* ref, gc** slot, gc* old, gc* value) {
  llvm_gcroot(ref, 0);
  llvm_gcroot(old, 0);
  llvm_gcroot(value, 0);
  bool res = JnJVM_org_j3_bindings_Bindings_writeBarrierCAS__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2(ref, slot, old, value);
  if (vmkit::Thread::get()->doYield) vmkit::Collector::collect();
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
