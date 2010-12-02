//===----------- MvmGC.h - Garbage Collection Interface -------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_MMAP_GC_H
#define MVM_MMAP_GC_H

#include "types.h"
#include "gcalloc.h"
#include "mvm/VirtualMachine.h"

#define gc_allocator std::allocator

using namespace mvm;

namespace mvm {
	class Thread;
	class VirtualMachine;

class GCVirtualTable : public CommonVirtualTable {
public:
  static uint32_t numberOfBaseFunctions() {
    return numberOfCommonEntries();
  }
  
  static uint32_t numberOfSpecializedTracers() {
    return 0;
  }
};

class Collector {
  friend class GCThread;
  friend class CollectionRV;
  static GCAllocator  *allocator;      /* The allocator */
  static SpinLock _globalLock;         /* Global lock for allocation */  

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
 
  
  enum { stat_collect, stat_alloc, stat_broken };

  static inline void  lock()   { _globalLock.lock(); }
  static inline void  unlock() { _globalLock.unlock(); }
  
  /* Interface for collection, verifies enable_collect */
  static void collect_unprotect();    
  /* The collection */  
  static void do_collect();           

  static inline GCChunkNode *o2node(const void *p) {
    if (!p) return 0;
    return GCHash::get(const_cast<void*>(p))->o2node(const_cast<void*>(p), GCChunkNode::maskCollectable);
  }

  static inline size_t real_nbb(GCChunkNode *n) { 
    return n->nbb() - sizeof(gcRoot);
  }
  

public:
  static void (*internMemoryError)(unsigned int);

  static int verbose; 
  
  static bool isLive(void* ptr, uintptr_t closure) {
    GCChunkNode *node = o2node(ptr);
    
    if(node && isMarked(node)) return true;
    else return false;
  }
  
  static void scanObject(void** ptr, uintptr_t closure);

  static void initialise();
  static void destroy();

  static inline void *allocate_unprotected(size_t sz) {
    return allocator->alloc(sz);
  }
  
  static inline void  free_unprotected(void *ptr) {
    allocator->free(ptr);
  }

  static inline void *begOf(const void *p) {
    GCChunkNode *node = o2node(p);
    if(node)
      return node->chunk();
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
#ifdef SERVICE
       (mvm::Thread::get()->MyVM->_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe))
#else
       (_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe))
#endif
      )
      collect(); 
  }

  static inline void *gcmalloc(VirtualTable *vt, size_t n) {
#if (__WORDSIZE == 64)
    void* res = malloc(n);
    memset(res, 0, n);
    ((void**)res)[0] = vt;
    return res;
#else
    lock();

#ifdef SERVICE
    if (threads->get_nb_threads()) {
      VirtualMachine* vm = mvm::Thread::get()->MyVM;
      vm->_since_last_collection -= n;
      if (_enable_auto && (vm->_since_last_collection <= 0)) {
        vm->gcTriggered++;
        if (vm->gcTriggered > vm->GCLimit) {
          vm->_since_last_collection += n;
          unlock();
          vm->stopService();
          return 0;
        }
        collect_unprotect();
      }
      
      if (vm->memoryUsed + n > vm->memoryLimit) {
        vm->_since_last_collection += n;
        unlock();
        vm->stopService();
        return 0;
      }
    } else {
#endif
    
    _since_last_collection -= n;
    if(_enable_auto && (_since_last_collection <= 0)) {
      collect_unprotect();
    }
#ifdef SERVICE
    }
#endif
    register GCChunkNode *header = allocator->alloc_chunk(n, 1, current_mark & 1);

#ifdef SERVICE
    if (threads->get_nb_threads()) {
      VirtualMachine* vm = mvm::Thread::get()->MyVM;
      header->meta = vm;
      vm->memoryUsed += n;
    }
#endif
    header->append(used_nodes);
    register struct gcRoot *p = header->chunk();
    p->setVirtualTable(vt);


    unlock();

    if (((CommonVirtualTable*)vt)->destructor)
      mvm::Thread::get()->MyVM->addFinalizationCandidate((gc*)p);


    return p;
#endif
  }

  static inline void *gcrealloc(void *ptr, size_t n) {
#if (__WORDSIZE == 64)
    void* res = realloc(ptr, n);
    return res;
#else
    lock();
    
    GCPage      *desc = GCHash::get(ptr);
    GCChunkNode  *node = desc->o2node(ptr, GCChunkNode::maskCollectable);

    if(!node)
      gcfatal("%p isn't a avalid object", ptr);

    size_t      old_sz = node->nbb();
#ifdef SERVICE
    if (threads->get_nb_threads()) {
      VirtualMachine* vm = mvm::Thread::get()->MyVM;
      vm->_since_last_collection -= (n - old_sz);
      if (_enable_auto && (vm->_since_last_collection <= 0)) {
        if (vm->gcTriggered + 1 > vm->GCLimit) {
          unlock();
          vm->stopService();
          return 0;
        }
        vm->gcTriggered++;
        collect_unprotect();
      }
      
      if (vm->memoryUsed + (n - old_sz) > vm->memoryLimit) {
        vm->_since_last_collection += (n - old_sz);
        unlock();
        vm->stopService();
        return 0;
      }
    } else {
#endif
    
    _since_last_collection -= (n - old_sz);

    if(_enable_auto && (_since_last_collection <= 0)) {
      collect_unprotect();
    }

#ifdef SERVICE
    }
#endif

    GCChunkNode  *res = allocator->realloc_chunk(desc, node, n);

#ifdef SERVICE
    if (threads->get_nb_threads()) {
      VirtualMachine* vm = mvm::Thread::get()->MyVM;
      res->meta = vm;
      vm->memoryUsed += (n - old_sz);
    }
#endif

    if(res != node) {
      res->append(used_nodes);
      mark(res);
    }

    gcRoot *obj = res->chunk();

    unlock();
    return obj;
#endif
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
    gcRoot *o = node->chunk();
    o->tracer(0);
  }

  static inline void markAndTrace(void* source, void *ptr, uintptr_t closure) {
    markAndTraceRoot(ptr, closure);
  }
  
  static inline void markAndTraceRoot(void *ptr, uintptr_t closure) {
    void* obj = *(void**)ptr;
    if (obj) {
      GCChunkNode *node = o2node(obj);
   
#ifdef WITH_LLVM_GCC
      assert(node && "No node in  precise mode");
      assert(obj == begOf(obj) && "Interior pointer");
#endif

      if(node && !isMarked(node)) {
        mark(node);
        node->remove();
        node->prepend(used_nodes);
      }
    }
  }

  static int getMaxMemory() {
    return 0;
  }
  
  static int getFreeMemory() {
    return 0;
  }
  
  static int getTotalMemory() {
    return 0;
  }

  void setMaxMemory(size_t sz){
  }

  void setMinMemory(size_t sz){
  }

  static gc* retainForFinalize(gc* val, uintptr_t closure) {
    markAndTraceRoot(&val, closure);
    return val;
  }
  
  static gc* retainReferent(gc* val, uintptr_t closure) {
    markAndTraceRoot(&val, closure);
    return val;
  }

  static gc* getForwardedReference(gc* val, uintptr_t closure) {
    return val;
  }
  
  static gc* getForwardedReferent(gc* val, uintptr_t closure) {
    return val;
  }
  
  static gc* getForwardedFinalizable(gc* val, uintptr_t closure) {
    return val;
  }
};

class collectable : public gcRoot {
public:
 
  size_t objectSize() const {
    return mvm::Collector::objectSize((void*)this);
  }

  void* operator new(size_t sz, VirtualTable *VT) {
    return mvm::Collector::gcmalloc(VT, sz);
  }

  void* operator new(size_t sz) {
    return malloc(sz);
  }

  void operator delete(void *) {
    gcfatal(0, "never call directly a destructor.....");
  }

  void* realloc(size_t n) {
    return mvm::Collector::gcrealloc(this, n);
  }

};
}

#endif
