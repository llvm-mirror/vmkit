//===---------------- gc.cc - Mvm Garbage Collector -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <setjmp.h>
#include <stdlib.h>

#include "mvm/GC/GC.h"
#include "gccollector.h"
#include "gcerror.h"

using namespace mvm;


typedef void (*memoryError_t)(unsigned int);

memoryError_t GCCollector::internMemoryError;

#ifndef MULTIPLE_GC

void gc::markAndTrace() const {
  GCCollector::markAndTrace((void*)this);
}

extern "C" void MarkAndTrace(gc* gc) {
  GCCollector::markAndTrace((void*)gc);
}

extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT) {
  return (gc*)GCCollector::gcmalloc(VT, sz);
}

size_t gc::objectSize() const {
  return GCCollector::objectSize((gc*)this);
}

void *gc::operator new(size_t sz, VirtualTable *vt) {
  return GCCollector::gcmalloc(vt, sz);
}

void *gc::operator new(size_t sz) {
  return malloc(sz);
}

void gc::operator delete(void *) { 
  gcfatal(0, "never call directly a destructor....."); 
}

void *gc::realloc(size_t n) {
  return GCCollector::gcrealloc(this, n);
}

#else

void gc::markAndTrace(Collector* GC) const {
  ((GCCollector*)GC)->markAndTrace((void*)this);
}

extern "C" void MarkAndTrace(gc* gc, Collector* GC) {
  ((GCCollector*)GC)->markAndTrace((void*)gc);
}

extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT, Collector* GC) {
  return (gc*)((GCCollector*)GC)->gcmalloc(VT, sz);
}

size_t gc::objectSize(Collector* GC) const {
  return ((GCCollector*)GC)->objectSize((gc*)this);
}

void *gc::operator new(size_t sz, VirtualTable *VT, Collector* GC) {
  return ((GCCollector*)GC)->gcmalloc(VT, sz);
}

void *gc::operator new(size_t sz, Collector* GC) {
  return malloc(sz);
}

void gc::operator delete(void *, Collector* GC) {
  gcfatal(0, "never call directly a destructor....."); 
}

void *gc::realloc(size_t n, Collector* GC) {
  return ((GCCollector*)GC)->gcrealloc(this, n);
}
#endif

unsigned int Collector::enable(unsigned int n) {
   return GCCollector::enable(n);
}

int Collector::isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                        int a3, int a4, int a5, int a6, int a7) {
  return GCCollector::isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
}

void Collector::die_if_sigsegv_occured_during_collection(void *addr) {
  GCCollector::die_if_sigsegv_occured_during_collection(addr);
}

void Collector::gcStats(size_t &no, size_t &nbb) {
  GCCollector::gcStats(&no, &nbb);
}

void Collector::initialise(markerFn marker) {
  GCCollector::initialise(marker);
}

void Collector::destroy() {
  GCCollector::destroy();
}

void Collector::inject_my_thread(mvm::Thread* th) {
#ifdef HAVE_PTHREAD
  GCCollector::inject_my_thread(th);
#endif
}

void Collector::maybeCollect() {
   GCCollector::maybeCollect();
}

void Collector::collect(void) {
   GCCollector::collect();
}

gc *Collector::begOf(const void *obj) {
  return (gc*)GCCollector::begOf((void*)obj);
}

int Collector::byteOffset(void *obj) {
  int beg = (intptr_t)GCCollector::begOf(obj);
  intptr_t off = (intptr_t)obj;
  return (off-beg);
}


void Collector::applyFunc(void (*func)(gcRoot *o, void *data), void *data) {
  return GCCollector::applyFunc(func, data);
}

int Collector::getMaxMemory(void){
  return 0;
}

int Collector::getFreeMemory(void){
  return 0;
}

int Collector::getTotalMemory(void){
  return 0;
}

void Collector::setMaxMemory(size_t sz){
}

void Collector::setMinMemory(size_t sz){
}

void Collector::registerMemoryError(void (*func)(unsigned int)){
  GCCollector::internMemoryError = func;
  //onMemoryError = &GCCollector::defaultMemoryError;
}

void Collector::remove_my_thread(mvm::Thread* th) {
#ifdef HAVE_PTHREAD
  GCCollector::remove_thread(th);
#endif
}

void GCThread::waitCollection() {
  mvm::Thread* th = mvm::Thread::get();
  unsigned int cm = GCCollector::current_mark;

  if(th != current_collector) {
    collectorGo();
    while((GCCollector::current_mark == cm) && 
          (GCCollector::status == GCCollector::stat_collect))
      _collectionCond.wait(&_stackLock);
  }
}

#ifdef HAVE_PTHREAD

void GCCollector::siggc_handler(int) {
  mvm::Thread* th = mvm::Thread::get();

  jmp_buf buf;
  setjmp(buf);
  
  GCCollector::threads->stackLock();
  
  if(!th) /* The thread is being destroyed */
    GCCollector::threads->another_mark();
  else {
    register unsigned int  **cur = (unsigned int**)(void*)&buf;
    register unsigned int  **max = (unsigned int**)th->baseSP;
    
    GCChunkNode *node;
    
    for(; cur<max; cur++) {
      if((node = o2node(*cur)) && (!GCCollector::isMarked(node))) {
        node->remove();
        node->append(GCCollector::used_nodes);
        GCCollector::mark(node);
      }
    }
    
    GCCollector::threads->another_mark();
    GCCollector::threads->waitCollection();
  }
  GCCollector::threads->stackUnlock();
}
#endif // HAVE_PTHREAD
