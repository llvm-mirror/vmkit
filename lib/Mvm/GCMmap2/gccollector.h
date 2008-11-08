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

#ifdef MULTIPLE_GC
#define STATIC
#else
#define STATIC static
#endif

#if defined(MULTIPLE_GC) && !defined(SERVICE_GC)
#define STATIC_FIELD
#else
#define STATIC_FIELD static
#endif

#if defined(MULTIPLE_GC)
#define STATIC_NO_SERVICE
#else
#define STATIC_NO_SERVICE static
#endif

namespace mvm {

class GCCollector : public Collector {
  friend class Collector;
#ifdef HAVE_PTHREAD
  friend class GCThread;
#endif
  STATIC_FIELD GCAllocator  *allocator;      /* The allocator */


  STATIC_FIELD GCChunkNode  *used_nodes;     /* Used memory nodes */
  STATIC_FIELD GCChunkNode  *unused_nodes;   /* Unused memory nodes */
  STATIC_FIELD unsigned int   current_mark;

  STATIC_NO_SERVICE int  _collect_freq_auto;      /* Collection frequency in gcmalloc/gcrealloc */
  STATIC_NO_SERVICE int  _collect_freq_maybe;     /* Collection frequency  in maybeCollect */
  STATIC_NO_SERVICE int  _since_last_collection;  /* Bytes left since last collection */
  STATIC_NO_SERVICE bool _enable_auto;            /* Automatic collection? */
  STATIC_NO_SERVICE bool _enable_maybe;           /* Collection in maybeCollect()? */
  STATIC_NO_SERVICE bool _enable_collection;      /* collection authorized? */
  STATIC_FIELD int  status;
  
  
  enum { stat_collect, stat_finalize, stat_alloc, stat_broken };

#ifdef HAVE_PTHREAD
  static void  siggc_handler(int);
  STATIC inline void  lock()   { threads->lock(); }
  STATIC inline void  unlock() { threads->unlock(); }
#else
  static void  siggc_handler(int) { }
  STATIC inline void  lock()   { }
  STATIC inline void  unlock() { }
#endif
  
  /* Interface for collection, verifies enable_collect */
  STATIC void collect_unprotect();    
  /* The collection */  
  STATIC void do_collect();           

  static inline GCChunkNode *o2node(void *p) {
    return GCHash::get(p)->o2node(p, GCChunkNode::maskCollectable);
  }

  static inline size_t real_nbb(GCChunkNode *n) { 
    return n->nbb() - sizeof(gc_header);
  }

public:
  STATIC_FIELD Collector::markerFn  _marker; /* The function which traces roots */
#ifdef SERVICE_GC
  static GCCollector* collectingGC;
#endif
#ifdef MULTIPLE_GC
  static GCCollector* bootstrapGC;
#endif
  STATIC_FIELD GCThread *threads;        /* le gestionnaire de thread et de synchro */
  static void (*internMemoryError)(unsigned int);

#ifdef HAVE_PTHREAD
  STATIC inline void  unlock_dont_recovery() { threads->unlock_dont_recovery(); }
  STATIC void die_if_sigsegv_occured_during_collection(void *addr);
#else
  STATIC void die_if_sigsegv_occured_during_collection(void *addr) { }
#endif

  STATIC void defaultMemoryError(unsigned int sz){
    unlock();
    internMemoryError(sz);
    lock();
  }

  STATIC void initialise(Collector::markerFn marker);
  STATIC void destroy();

  static int siggc();

#ifdef HAVE_PTHREAD
  STATIC void inject_my_thread(mvm::Thread* th);
  STATIC inline void  remove_thread(mvm::Thread* th) {
    threads->remove(th);
  }
  STATIC inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                             int a3, int a4, int a5, int a6, int a7) {
    return threads->isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
  }
#else
  STATIC inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2,
                             int a3, int a4, int a5, int a6, int a7) {
    return 0;
  }
#endif

  STATIC inline void *allocate_unprotected(size_t sz) {
    return allocator->alloc(sz);
  }
  
  STATIC inline void  free_unprotected(void *ptr) {
    allocator->free(ptr);
  }

  static inline void *begOf(void *p) {
    GCChunkNode *node = o2node(p);
    if(node)
      return node->chunk()->_2gc();
    else
      return 0;
  }

  STATIC void gcStats(size_t *no, size_t *nbb);

  static inline size_t objectSize(void *ptr) {
    GCChunkNode *node = o2node(ptr);
    return node ? real_nbb(node) : 0;
  }

  STATIC inline void collect() {
    lock();
    collect_unprotect();
    unlock();
  }

  STATIC inline void maybeCollect() {
    if(_enable_auto && 
       (_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe)))
      collect(); 
  }

  STATIC inline void *gcmalloc(VirtualTable *vt, size_t n) {
    lock();
    
    _since_last_collection -= n;
    if(_enable_auto && (_since_last_collection <= 0)) {
#ifdef SERVICE_GC
      ++gcTriggered;
#endif
      collect_unprotect();
    }
    
    register GCChunkNode *header = allocator->alloc_chunk(n + sizeof(gc_header), 1, current_mark & 1);
#ifdef SERVICE_GC
    header->meta = this;
    memoryUsed += n;
#endif
    header->append(used_nodes);
    //printf("Allocate %d bytes at %p [%p] %d %d\n", n, header->chunk()->_2gc(),
    //       header, header->nbb(), real_nbb(header));
    register struct gc_header *p = header->chunk();
    p->_XXX_vt = vt;


    unlock();
    return p->_2gc();
  }

  STATIC inline void *gcrealloc(void *ptr, size_t n) {
    lock();
    
    GCPage      *desc = GCHash::get(ptr);
    GCChunkNode  *node = desc->o2node(ptr, GCChunkNode::maskCollectable);

    if(!node)
      gcfatal("%p isn't a avalid object", ptr);

    size_t      old_sz = node->nbb();
    
    _since_last_collection -= (n - old_sz);

    if(_enable_auto && (_since_last_collection <= 0)) {
#ifdef SERVICE_GC
      ++gcTriggered;
#endif
      collect_unprotect();
    }

    GCChunkNode  *res = allocator->realloc_chunk(desc, node, n+sizeof(gc_header));
#ifdef SERVICE_GC
    res->meta = this;
    memoryUsed += (n - old_sz);
#endif

    if(res != node) {
      res->append(used_nodes);
      mark(res);
    }

    gc_header *obj = res->chunk();

    unlock();
    return obj->_2gc();
  }

  STATIC inline unsigned int enable(unsigned int n)  {
    register unsigned int old = _enable_collection;
    _enable_collection = n; 
    return old;
  }

  STATIC inline bool isMarked(GCChunkNode *node) { 
    return node->mark() == (current_mark & 1);
  }
  
  STATIC inline void mark(GCChunkNode *node) {
    node->_mark(current_mark & 1);
  }

  STATIC inline void trace(GCChunkNode *node) {
    gc_header *o = node->chunk();
#ifdef MULTIPLE_GC
    o->_2gc()->tracer(this);
#else
    o->_2gc()->tracer();
#endif
    markAndTrace(o);
  }

  STATIC inline void markAndTrace(void *ptr) {
    GCChunkNode *node = o2node(ptr);

    if(node && !isMarked(node)) {
      mark(node);
      node->remove();
      node->append(used_nodes);
      trace(node);
    }
  }

  STATIC inline void applyFunc(void (*func)(gcRoot *o, void *data), void *data){
    lock(); /* Make sure no one collects or allocates */
    status = stat_collect; /* Forbids collections */
    GCChunkNode *cur=used_nodes->next(); /* Get starter */
    unlock(); /* One can allocate now */ 

    for(; cur!=used_nodes; cur=cur->next())
      func(cur->chunk()->_2gc(), data);
    
    /* No need to lock ! */
    status = stat_alloc; 
  }
};

}
#endif
