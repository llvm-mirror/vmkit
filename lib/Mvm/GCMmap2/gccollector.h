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

#include "config.h"
#include "gcalloc.h"
#ifdef HAVE_PTHREAD
#include "gcthread.h"
#endif
#include "mvm/GC/GC.h"

namespace mvm {

class GCCollector : public Collector {
#ifdef HAVE_PTHREAD
  friend class GCThread;
#endif
  GCAllocator  *allocator;      /* The allocator */

  Collector::markerFn  _marker; /* The function which traces roots */

  GCChunkNode  *used_nodes;     /* Used memory nodes */
  GCChunkNode  *unused_nodes;   /* Unused memory nodes */

  unsigned int   current_mark;

  int  _collect_freq_auto;      /* Collection frequency in gcmalloc/gcrealloc */
  int  _collect_freq_maybe;     /* Collection frequency  in maybeCollect */
  int  _since_last_collection;  /* Bytes left since last collection */
  bool _enable_auto;            /* Automatic collection? */
  bool _enable_maybe;           /* Collection in maybeCollect()? */
  bool _enable_collection;      /* collection authorized? */
  int  status;

  enum { stat_collect, stat_finalize, stat_alloc, stat_broken };

#ifdef HAVE_PTHREAD
  static void  siggc_handler(int);
  inline void  lock()   { threads->lock(); }
  inline void  unlock() { threads->unlock(); }
#else
  static void  siggc_handler(int) { }
  inline void  lock()   { }
  inline void  unlock() { }
#endif
  
  /* Interface for collection, verifies enable_collect */
  void collect_unprotect();    
  /* The collection */  
  void do_collect();           

  static inline GCChunkNode *o2node(void *p) {
    return GCHash::get(p)->o2node(p, GCChunkNode::maskCollectable);
  }

  static inline size_t real_nbb(GCChunkNode *n) { 
    return n->nbb() - sizeof(gc_header);
  }

public:
  GCThread *threads;        /* le gestionnaire de thread et de synchro */
  static void (*internMemoryError)(unsigned int);

#ifdef HAVE_PTHREAD
  inline void  unlock_dont_recovery() { threads->unlock_dont_recovery(); }
  void die_if_sigsegv_occured_during_collection(void *addr);
#else
  void die_if_sigsegv_occured_during_collection(void *addr) { }
#endif

  void defaultMemoryError(unsigned int sz){
    unlock();
    internMemoryError(sz);
    lock();
  }

  void initialise(Collector::markerFn marker);
  void destroy();

  static int siggc();

#ifdef HAVE_PTHREAD
  void inject_my_thread(void *base_sp);
  inline void  remove_thread(GCThreadCollector *loc) {
    threads->remove(loc);
  }
  inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, 
                      int a3, int a4, int a5, int a6, int a7) {
    return threads->isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
  }
#else
  inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2,
                      int a3, int a4, int a5, int a6, int a7) {
    return 0;
  }
#endif

  inline void *allocate_unprotected(size_t sz) {
    return allocator->alloc(sz);
  }
  
  inline void  free_unprotected(void *ptr) {
    allocator->free(ptr);
  }

  static inline gc *begOf(void *p) {
    GCChunkNode *node = o2node(p);
    if(node)
      return node->chunk()->_2gc();
    else
      return 0;
  }

  void gcStats(size_t *no, size_t *nbb);

  static inline size_t objectSize(void *ptr) {
    GCChunkNode *node = o2node(ptr);
    return node ? real_nbb(node) : 0;
  }

  inline void collect() {
    lock();
    collect_unprotect();
    unlock();
  }

  inline void maybeCollect() {
    if(_enable_auto && 
       (_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe)))
      collect(); 
  }

  inline gc *gcmalloc(VirtualTable *vt, size_t n) {
    lock();
    register GCChunkNode *header = allocator->alloc_chunk(n + sizeof(gc_header), 1, current_mark & 1);
    
    header->append(used_nodes);
    //printf("Allocate %d bytes at %p [%p] %d %d\n", n, header->chunk()->_2gc(),
    //       header, header->nbb(), real_nbb(header));
    
    register struct gc_header *p = header->chunk();
    p->_XXX_vt = vt;

    _since_last_collection -= n;
    if(_enable_auto && (_since_last_collection <= 0))
      collect_unprotect();

    unlock();
    return p->_2gc();
  }

  inline gc *gcrealloc(void *ptr, size_t n) {
    lock();

    GCPage      *desc = GCHash::get(ptr);
    GCChunkNode  *node = desc->o2node(ptr, GCChunkNode::maskCollectable);

    if(!node)
      gcfatal("%p isn't a avalid object", ptr);

    size_t      old_sz = node->nbb();
    GCChunkNode  *res = allocator->realloc_chunk(desc, node, n+sizeof(gc_header));

    if(res != node) {
      res->append(used_nodes);
      mark(res);
    }

    gc_header *obj = res->chunk();
     _since_last_collection -= (n - old_sz);

    if(_enable_auto && (_since_last_collection <= 0))
      collect_unprotect();

    unlock();
    return obj->_2gc();
  }

  inline unsigned int enable(unsigned int n)  {
    register unsigned int old = _enable_collection;
    _enable_collection = n; 
    return old;
  }

  inline bool isMarked(GCChunkNode *node) { 
    return node->mark() == (current_mark & 1);
  }
  
  inline void mark(GCChunkNode *node) {
    node->_mark(current_mark & 1);
  }

  inline void trace(GCChunkNode *node) {
    gc_header *o = node->chunk();
    o->_2gc()->tracer(real_nbb(node));
    markAndTrace(o);
  }

  inline void markAndTrace(void *ptr) {
    GCChunkNode *node = o2node(ptr);

    if(node && !isMarked(node)) {
      mark(node);
      node->remove();
      node->append(used_nodes);
      trace(node);
    }
  }

  inline void applyFunc(void (*func)(gc *o, void *data), void *data){
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
