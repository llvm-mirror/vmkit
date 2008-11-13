//===------------ gccollector.h - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_COLLECTOR_H_
#define _GC_COLLECTOR_H_

#include "mvm/Config/config.h"
#include "gcalloc.h"
#ifdef HAVE_PTHREAD
#include "gcthread.h"
#endif
#include "mvm/GC/GC.h"

namespace mvm {

class GCCollector : public Collector {
  friend class Collector;
#ifdef HAVE_PTHREAD
  friend class GCThread;
#endif
  static GCAllocator  *allocator;      /* The allocator */


  static GCChunkNode  *used_nodes;     /* Used memory nodes */
  static GCChunkNode  *unused_nodes;   /* Unused memory nodes */
  static unsigned int   current_mark;

  static int  _collect_freq_auto;      /* Collection frequency in gcmalloc/gcrealloc */
  static int  _collect_freq_maybe;     /* Collection frequency  in maybeCollect */
  static int  _since_last_collection;  /* Bytes left since last collection */
  static bool _enable_auto;            /* Automatic collection? */
  static bool _enable_maybe;           /* Collection in maybeCollect()? */
  static bool _enable_collection;      /* collection authorized? */
  static int  status;
  
  
  enum { stat_collect, stat_finalize, stat_alloc, stat_broken };

#ifdef HAVE_PTHREAD
  static void  siggc_handler(int);
  static inline void  lock()   { threads->lock(); }
  static inline void  unlock() { threads->unlock(); }
#else
  static void  siggc_handler(int) { }
  static inline void  lock()   { }
  static inline void  unlock() { }
#endif
  
  /* Interface for collection, verifies enable_collect */
  static void collect_unprotect();    
  /* The collection */  
  static void do_collect();           

  static inline GCChunkNode *o2node(void *p) {
    return GCHash::get(p)->o2node(p, GCChunkNode::maskCollectable);
  }

  static inline size_t real_nbb(GCChunkNode *n) { 
    return n->nbb() - sizeof(gc_header);
  }

public:
  static Collector::markerFn  _marker; /* The function which traces roots */
  static GCThread *threads;        /* le gestionnaire de thread et de synchro */
  static void (*internMemoryError)(unsigned int);

#ifdef HAVE_PTHREAD
  static inline void  unlock_dont_recovery() { threads->unlock_dont_recovery(); }
  static void die_if_sigsegv_occured_during_collection(void *addr);
#else
  static void die_if_sigsegv_occured_during_collection(void *addr) { }
#endif

  static void defaultMemoryError(unsigned int sz){
    unlock();
    internMemoryError(sz);
    lock();
  }

  static void initialise(Collector::markerFn marker);
  static void destroy();

  static int siggc();

#ifdef HAVE_PTHREAD
  static void inject_my_thread(mvm::Thread* th);
  static inline void  remove_thread(mvm::Thread* th) {
    threads->remove(th);
  }
  static inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                             int a3, int a4, int a5, int a6, int a7) {
    return threads->isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
  }
#else
  static inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2,
                             int a3, int a4, int a5, int a6, int a7) {
    return 0;
  }
#endif

  static inline void *allocate_unprotected(size_t sz) {
    return allocator->alloc(sz);
  }
  
  static inline void  free_unprotected(void *ptr) {
    allocator->free(ptr);
  }

  static inline void *begOf(void *p) {
    GCChunkNode *node = o2node(p);
    if(node)
      return node->chunk()->_2gc();
    else
      return 0;
  }

  static void gcStats(size_t *no, size_t *nbb);

  static inline size_t objectSize(void *ptr) {
    GCChunkNode *node = o2node(ptr);
    return node ? real_nbb(node) : 0;
  }

  static inline void collect() {
    lock();
    collect_unprotect();
    unlock();
  }

  static inline void maybeCollect() {
    if(_enable_auto && 
       (_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe)))
      collect(); 
  }

  static inline void *gcmalloc(VirtualTable *vt, size_t n) {
    lock();
    
    _since_last_collection -= n;
    if(_enable_auto && (_since_last_collection <= 0)) {
#ifdef SERVICE
      mvm::Thread::get()->vm->gcTriggered++;
#endif
      collect_unprotect();
    }
    
    register GCChunkNode *header = allocator->alloc_chunk(n + sizeof(gc_header), 1, current_mark & 1);
#ifdef SERVICE
    VirtualMachine* vm = mvm::Thread::get()->vm;
    header->meta = vm;
    vm->memoryUsed += n;
#endif
    header->append(used_nodes);
    //printf("Allocate %d bytes at %p [%p] %d %d\n", n, header->chunk()->_2gc(),
    //       header, header->nbb(), real_nbb(header));
    register struct gc_header *p = header->chunk();
    p->_XXX_vt = vt;


    unlock();
    return p->_2gc();
  }

  static inline void *gcrealloc(void *ptr, size_t n) {
    lock();
    
    GCPage      *desc = GCHash::get(ptr);
    GCChunkNode  *node = desc->o2node(ptr, GCChunkNode::maskCollectable);

    if(!node)
      gcfatal("%p isn't a avalid object", ptr);

    size_t      old_sz = node->nbb();
    
    _since_last_collection -= (n - old_sz);

    if(_enable_auto && (_since_last_collection <= 0)) {
#ifdef SERVICE
      mvm::Thread::get()->vm->gcTriggered++;
#endif
      collect_unprotect();
    }

    GCChunkNode  *res = allocator->realloc_chunk(desc, node, n+sizeof(gc_header));

#ifdef SERVICE
    VirtualMachine* vm = mvm::Thread::get()->vm;
    header->meta = vm;
    vm->memoryUsed += (n - old_sz);
#endif

    if(res != node) {
      res->append(used_nodes);
      mark(res);
    }

    gc_header *obj = res->chunk();

    unlock();
    return obj->_2gc();
  }

  static inline unsigned int enable(unsigned int n)  {
    register unsigned int old = _enable_collection;
    _enable_collection = n; 
    return old;
  }

  static inline bool isMarked(GCChunkNode *node) { 
    return node->mark() == (current_mark & 1);
  }
  
  static inline void mark(GCChunkNode *node) {
    node->_mark(current_mark & 1);
  }

  static inline void trace(GCChunkNode *node) {
    gc_header *o = node->chunk();
    o->_2gc()->tracer();
    markAndTrace(o);
  }

  static inline void markAndTrace(void *ptr) {
    GCChunkNode *node = o2node(ptr);

    if(node && !isMarked(node)) {
      mark(node);
      node->remove();
      node->append(used_nodes);
      trace(node);
    }
  }

};

}
#endif
