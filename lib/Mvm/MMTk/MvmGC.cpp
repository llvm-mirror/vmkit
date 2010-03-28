//===----------- MvmGC.cpp - Garbage Collection Interface -----------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MvmGC.h"

#include <set>

using namespace mvm;

static mvm::SpinLock lock;
std::set<gc*> __InternalSet__;

extern "C" void* gcmalloc(uint32_t sz, void* _VT) {
  gc* res = 0;
  VirtualTable* VT = (VirtualTable*)_VT;
  sz = llvm::RoundUpToAlignment(sz, sizeof(void*));
  res = (gc*)malloc(sz);
  memset(res, 0, sz);
  
  lock.acquire();
  __InternalSet__.insert(res);
  lock.release();
  
  res->setVirtualTable(VT);
  return res;
}

extern "C" void* gcmallocUnresolved(uint32_t sz, VirtualTable* VT) {
  gc* res = (gc*)gcmalloc(sz, VT);
  if (VT->destructor)
    mvm::Thread::get()->MyVM->addFinalizationCandidate(res);
  return res;
}

extern "C" void addFinalizationCandidate(gc* obj) {
  mvm::Thread::get()->MyVM->addFinalizationCandidate(obj);
}

extern "C" void* AllocateMagicArray(int32_t sz, void* length) {
  gc* res = (gc*)malloc(sz);
  memset(res, 0, sz);
  ((void**)res)[0] = length;
  return res;
}

void* Collector::begOf(gc* obj) {
  lock.acquire();
  std::set<gc*>::iterator I = __InternalSet__.find(obj);
  std::set<gc*>::iterator E = __InternalSet__.end();
  lock.release();
    
  if (I != E) return obj;
  return 0;
}

void MutatorThread::init(Thread* _th) {
  MutatorThread* th = (MutatorThread*)_th;
  th->realRoutine(_th);
}

bool Collector::isLive(gc* ptr) {
  abort();
  return false;
}

void Collector::scanObject(void** ptr) {
  abort();
}
 
void Collector::markAndTrace(void* source, void* ptr) {
  abort();
}
  
void Collector::markAndTraceRoot(void* ptr) {
  abort();
}

gc* Collector::retainForFinalize(gc* val) {
  abort();
  return NULL;
}
  
gc* Collector::retainReferent(gc* val) {
  abort();
  return NULL;
}
  
gc* Collector::getForwardedFinalizable(gc* val) {
  abort();
  return NULL;
}
  
gc* Collector::getForwardedReference(gc* val) {
  abort();
  return NULL;
}
  
gc* Collector::getForwardedReferent(gc* val) {
  abort();
  return NULL;
}

void Collector::collect() {
  // Do nothing.
}

void Collector::initialise() {
}
