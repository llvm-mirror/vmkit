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

#ifdef MULTIPLE_GC
#define COLLECTOR ((GCCollector*)(mvm::Thread::get()->GC))->
#else
#define COLLECTOR GCCollector::
#endif


typedef void (*memoryError_t)(unsigned int);

memoryError_t GCCollector::internMemoryError;

#ifdef MULTIPLE_GC
void gc::markAndTrace(Collector* GC) const {
  ((GCCollector*)GC)->markAndTrace((void*)this);
}
#else
void gc::markAndTrace() const {
  GCCollector::markAndTrace((void*)this);
}
#endif

size_t gc::objectSize() const {
  return COLLECTOR objectSize((gc*)this);
}

void *gc::operator new(size_t sz, VirtualTable *vt) {
  return COLLECTOR gcmalloc(vt, sz);
}

void *gc::operator new(size_t sz) {
  return malloc(sz);
}

void gc::operator delete(void *) { 
  gcfatal(0, "never call directly a destructor....."); 
}

void *gc::realloc(size_t n) {
  return COLLECTOR gcrealloc(this, n);
}


#ifdef MULTIPLE_GC

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

#undef COLLECTOR
#ifdef MULTIPLE_GC
#define COLLECTOR ((GCCollector*)this)->
#else
#define COLLECTOR GCCollector::
#endif

unsigned int Collector::enable(unsigned int n) {
   return COLLECTOR enable(n);
}

int Collector::isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                        int a3, int a4, int a5, int a6, int a7) {
  return COLLECTOR isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
}

void Collector::die_if_sigsegv_occured_during_collection(void *addr) {
  COLLECTOR die_if_sigsegv_occured_during_collection(addr);
}

void Collector::gcStats(size_t &no, size_t &nbb) {
  COLLECTOR gcStats(&no, &nbb);
}

void Collector::initialise(markerFn marker, void *base_sp) {
#ifdef MULTIPLE_GC
  GCCollector* GC = new GCCollector();
  GCCollector::bootstrapGC = GC;
  mvm::Thread::get()->GC = GC;
  GC->initialise(marker);
  GC->inject_my_thread(base_sp);
#else
  GCCollector::initialise(marker);
  GCCollector::inject_my_thread(base_sp);
#endif
}

void Collector::destroy() {
  COLLECTOR destroy();
}

void Collector::inject_my_thread(void *base_sp) {
#ifdef HAVE_PTHREAD
  COLLECTOR inject_my_thread(base_sp);
#endif
}

void Collector::maybeCollect() {
   COLLECTOR maybeCollect();
}

void Collector::collect(void) {
   COLLECTOR collect();
}

gc *Collector::begOf(const void *obj) {
  return (gc*)COLLECTOR begOf((void*)obj);
}

int Collector::byteOffset(void *obj) {
  int beg = (intptr_t)COLLECTOR begOf(obj);
  intptr_t off = (intptr_t)obj;
  return (off-beg);
}


void Collector::applyFunc(void (*func)(gcRoot *o, void *data), void *data) {
  return COLLECTOR applyFunc(func, data);
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
  COLLECTOR internMemoryError = func;
  //onMemoryError = &COLLECTOR defaultMemoryError;
}

void Collector::remove_my_thread() {
#ifdef HAVE_PTHREAD
  COLLECTOR remove_thread(COLLECTOR threads->myloc());
#endif
}

#undef COLLECTOR
void GCThread::waitCollection() {
#if defined(MULTIPLE_GC)
#if defined(SERVICE_GC)
  GCCollector* GC = GCCollector::collectingGC;
#else
  GCCollector* GC = ((GCCollector*)mvm::Thread::get()->GC);
#endif
#define COLLECTOR GC->
#else
#define COLLECTOR GCCollector::
#endif
  unsigned int cm = COLLECTOR current_mark;

  if(Thread::self() != collector_tid) {
    collectorGo();
    while((COLLECTOR current_mark == cm) && 
          (COLLECTOR status == COLLECTOR stat_collect))
      _collectionCond.wait(&_stackLock);
  }
}

#ifdef HAVE_PTHREAD

#undef COLLECTOR
void GCCollector::siggc_handler(int) {
#if defined(MULTIPLE_GC)
#if defined(SERVICE_GC)
  GCCollector* GC = collectingGC;
#else
  GCCollector* GC = ((GCCollector*)mvm::Thread::get()->GC);
#endif
#define COLLECTOR GC->
#else
#define COLLECTOR GCCollector::
#endif
  GCThreadCollector     *loc = COLLECTOR threads->myloc();
  
  jmp_buf buf;
  setjmp(buf);
  
  COLLECTOR threads->stackLock();
  
  if(!loc) /* a key is being destroyed */  
    COLLECTOR threads->another_mark();
  else {
     register unsigned int  **cur = (unsigned int **)&cur;
     register unsigned int  **max = loc->base_sp();
    
     GCChunkNode *node;
    
     for(; cur<max; cur++) {
       if((node = o2node(*cur)) && (!COLLECTOR isMarked(node))) {
         node->remove();
         node->append(COLLECTOR used_nodes);
         COLLECTOR mark(node);
       }
     }
    
     COLLECTOR threads->another_mark();
    COLLECTOR threads->waitCollection();
  }
  COLLECTOR threads->stackUnlock();
#undef COLLECTOR
}
#endif // HAVE_PTHREAD

#ifdef MULTIPLE_GC
Collector* Collector::allocate() {
  GCCollector* GC = new GCCollector();
  GC->initialise(GCCollector::bootstrapGC->_marker);
  return GC;
}
#endif // MULTIPLE_GC

#ifdef SERVICE_GC
Collector* Collector::getCollectorFromObject(const void* o) {
  return (Collector*)GCCollector::o2node((void*)o)->meta;
}
#endif

#ifdef MULTIPLE_GC
extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT, Collector* GC) {
  return (gc*)((GCCollector*)GC)->gcmalloc(VT, sz);
}
#else
extern "C" gc* gcmalloc(size_t sz, VirtualTable* VT) {
  return (gc*)GCCollector::gcmalloc(VT, sz);
}
#endif

