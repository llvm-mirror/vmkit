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

typedef void (*memoryError_t)(unsigned int);

memoryError_t GCCollector::internMemoryError;

void gc::markAndTrace() const {
 	GCCollector::markAndTrace((void*)this);
}

size_t gc::objectSize() const {
 	return GCCollector::objectSize((gc*)this);
}

void *gc::operator new(size_t sz, VirtualTable *vt) {
 	return GCCollector::gcmalloc(vt, sz);
}

void gc::operator delete(void *) { 
	gcfatal(0, "never call directly a destructor....."); 
}


void *gc::realloc(size_t n) {
 	return GCCollector::gcrealloc(this, n);
}

unsigned int gc::enable(unsigned int n) {
 	return GCCollector::enable(n);
}

int gc::isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
	return GCCollector::isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
}

void gc::die_if_sigsegv_occured_during_collection(void *addr) {
	GCCollector::die_if_sigsegv_occured_during_collection(addr);
}

void gc::gcStats(size_t &no, size_t &nbb) {
	GCCollector::gcStats(&no, &nbb);
}

void gc::initialise(markerFn marker, void *base_sp) {
#ifdef HAVE_PTHREAD
	Thread::initialise();
#endif
	GCCollector::initialise(marker);
#ifdef HAVE_PTHREAD
	GCCollector::inject_my_thread(base_sp);
#endif
}

void gc::destroy() {
	GCCollector::destroy();
}

void gc::inject_my_thread(void *base_sp) {
#ifdef HAVE_PTHREAD
	GCCollector::inject_my_thread(base_sp);
#endif
}

void gc::maybeCollect() {
 	GCCollector::maybeCollect();
}

void gc::collect(void) {
 	GCCollector::collect();
}

gc *gc::begOf(const void *obj) {
	return GCCollector::begOf((void*)obj);
}

int gc::byteOffset(void *obj) {
	int beg = (int)GCCollector::begOf(obj);
  int off = (int)obj;
  return (off-beg);
}


void gc::applyFunc(void (*func)(gc *o, void *data), void *data){
  return GCCollector::applyFunc(func, data);
}

int gc::getMaxMemory(void){
  return 0;
}

int gc::getFreeMemory(void){
  return 0;
}

int gc::getTotalMemory(void){
  return 0;
}

void gc::setMaxMemory(size_t sz){
}

void gc::setMinMemory(size_t sz){
}

void gc::registerMemoryError(void (*func)(unsigned int)){
  GCCollector::internMemoryError = func;
  onMemoryError = &GCCollector::defaultMemoryError;
  
}

void gc::remove_my_thread() {
#ifdef HAVE_PTHREAD
  GCCollector::remove_thread(GCCollector::threads->myloc());
#endif
}

