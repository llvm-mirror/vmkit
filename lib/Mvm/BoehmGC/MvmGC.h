//===----------- MvmGC.h - Garbage Collection Interface -------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_BOEHM_GC_H
#define MVM_BOEHM_GC_H

//#define GC_REDIRECT_TO_LOCAL
#include <stdlib.h>
#include <gc/gc_allocator.h>
//#include "gc/gc_local_alloc.h"
#include "gc/gc.h"

#define STATIC_TRACER(type) staticTracer(type* obj)
#define TRACER tracer()
#define PARENT_TRACER tracer()
#define MARK_AND_TRACE markAndTrace()
#define CALL_TRACER tracer()

namespace mvm {
    class Thread;
}

extern "C" void * GC_dlopen(const char *path, int mode) throw ();

#define  gc_new(Class)  __gc_new(Class::VT) Class
#define __gc_new new

namespace mvm {
class collectable : public gcRoot {
public:

  void markAndTrace() const {}

  size_t objectSize() const {
    gc_header * res = (gc_header*)(GC_base((void*)this));
    return (GC_size(res) - sizeof(gc_header));
  }

  void* operator new(size_t sz, VirtualTable *VT) {
    gc_header * res = (gc_header*) GC_MALLOC(sz + sizeof(gc_header));
    res -> _XXX_vt= VT;
    
    destructor_t dest = res->getDestructor();
    if (dest)
      GC_register_finalizer_no_order(res, (void (*)(void*, void*))dest, NULL,
                                     NULL, NULL); 
    return res->_2gc();
  }

  void* operator new(size_t sz) {
    return malloc(sz);
  }

  void operator delete(void * p) {
    //GC_FREE(p);
  }

  void* realloc(size_t n) {
    void * old = GC_base(this);
    gc_header * res = (gc_header*) GC_REALLOC(old, n + sizeof(gc_header));
    return res->_2gc();
  }

};
  
static int maxMem = 0;

class Collector {
public:

  static void initialise();
  static void destroy() {}

  static unsigned int enable(unsigned int n) {
    int old = GC_dont_gc;
    if(n)
      GC_enable();
    else
      GC_disable();
    return !old;
  }

  static void gcStats(size_t &no, size_t &nbb) {
    no = 0;
    nbb = GC_get_heap_size(); 
  }
  
  static void maybeCollect() {
    GC_collect_a_little();
  }

  static void collect(void) {
    GC_gcollect();
  }
  
  static void inject_my_thread(mvm::Thread*);
  
  static void remove_my_thread(mvm::Thread*) {}
  static Collector* allocate() { return 0; }

  static gc* begOf(const void *obj) {
    gc_header * a = (gc_header*)GC_base((void*)obj);
    if(a == NULL) return NULL;
    else return (gc*)a->_2gc();
  }

  inline static bool isObject(const void *o) {
    return begOf((void*)o);
  }
  

  static int getMaxMemory(void) { return maxMem; }
  
  static int getFreeMemory(void) {
    return GC_get_free_bytes(); 
  }

  static int getTotalMemory(void) {
    return GC_get_heap_size();
  }
  
  static void setMaxMemory(size_t size) {
    GC_set_max_heap_size(size);
    maxMem = size;
  }
  
  static void     setMinMemory(size_t size) {
    if(GC_get_heap_size() < size)
    GC_expand_hp(size - GC_get_heap_size());
  }

  static bool isLive() {
    return true;
  }
};
}

#endif
