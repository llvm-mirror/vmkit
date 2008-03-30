//===---------------- gc.cc - Mvm Garbage Collector -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "gccollector.h"
#include "gcerror.h"

using namespace mvm;

static GCCollector TheCollector;

typedef void (*memoryError_t)(unsigned int);

memoryError_t GCCollector::internMemoryError;

void gc::markAndTrace() const {
 	TheCollector.markAndTrace((void*)this);
}

size_t gc::objectSize() const {
 	return GCCollector::objectSize((gc*)this);
}

void *gc::operator new(size_t sz, VirtualTable *vt) {
 	return TheCollector.gcmalloc(vt, sz);
}

void gc::operator delete(void *) { 
	gcfatal(0, "never call directly a destructor....."); 
}

void *gc::realloc(size_t n) {
 	return TheCollector.gcrealloc(this, n);
}

unsigned int Collector::enable(unsigned int n) {
 	return TheCollector.enable(n);
}

int Collector::isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                        int a3, int a4, int a5, int a6, int a7) {
  return TheCollector.isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
}

void Collector::die_if_sigsegv_occured_during_collection(void *addr) {
  TheCollector.die_if_sigsegv_occured_during_collection(addr);
}

void Collector::gcStats(size_t &no, size_t &nbb) {
	TheCollector.gcStats(&no, &nbb);
}

void Collector::initialise(markerFn marker, void *base_sp) {
#ifdef HAVE_PTHREAD
	Thread::initialise();
#endif
	TheCollector.initialise(marker);
#ifdef HAVE_PTHREAD
	TheCollector.inject_my_thread(base_sp);
#endif
}

void Collector::destroy() {
	TheCollector.destroy();
}

void Collector::inject_my_thread(void *base_sp) {
#ifdef HAVE_PTHREAD
	TheCollector.inject_my_thread(base_sp);
#endif
}

void Collector::maybeCollect() {
 	TheCollector.maybeCollect();
}

void Collector::collect(void) {
 	TheCollector.collect();
}

gc *Collector::begOf(const void *obj) {
	return GCCollector::begOf((void*)obj);
}

int Collector::byteOffset(void *obj) {
	int beg = (intptr_t)GCCollector::begOf(obj);
  intptr_t off = (intptr_t)obj;
  return (off-beg);
}


void Collector::applyFunc(void (*func)(gc *o, void *data), void *data) {
  return TheCollector.applyFunc(func, data);
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

void Collector::remove_my_thread() {
#ifdef HAVE_PTHREAD
  TheCollector.remove_thread(TheCollector.threads->myloc());
#endif
}

#ifdef HAVE_PTHREAD
void GCCollector::siggc_handler(int) {
 	GCThreadCollector     *loc = TheCollector.threads->myloc();
 	register unsigned int cm = TheCollector.current_mark;
	//	jmp_buf buf;

	//	setjmp(buf);

	TheCollector.threads->stackLock();
	
 	if(!loc) /* a key is being destroyed */	
 		TheCollector.threads->another_mark();
 	else if(loc->current_mark() != cm) {
 		register unsigned int	**cur = (unsigned int **)&cur;
 		register unsigned int	**max = loc->base_sp();
		
 		GCChunkNode *node;
		
 		for(; cur<max; cur++) {
 			if((node = o2node(*cur)) && (!TheCollector.isMarked(node))) {
 				node->remove();
 				node->append(TheCollector.used_nodes);
 				TheCollector.mark(node);
 			}
 		}
		
 		loc->current_mark(cm);
 		TheCollector.threads->another_mark();
		
		TheCollector.threads->waitCollection();
	}
	TheCollector.threads->stackUnlock();
}
#endif

void GCThread::waitCollection() {
	unsigned int cm = TheCollector.current_mark;

	if(Thread::self() != collector_tid) {
		collectorGo();
		while((TheCollector.current_mark == cm) && 
          (TheCollector.status == GCCollector::stat_collect))
			_collectionCond.wait(&_stackLock);
	}
}
