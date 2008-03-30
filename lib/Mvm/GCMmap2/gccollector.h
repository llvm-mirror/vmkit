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

class GCCollector {
#ifdef HAVE_PTHREAD
	friend class GCThread;

#endif
	static GCAllocator  *allocator;      /* la machine à allouer de la mémoire */

 	static Collector::markerFn  _marker;        /* la fonction qui marque les racines */

 	static GCChunkNode  *used_nodes;     /* les noeuds mémoires utilisés */
	static GCChunkNode  *unused_nodes;   /* les noeuds inutilisés */

 	static unsigned int	 current_mark;

 	static int	_collect_freq_auto;     /* fréquence de collection automatique (lors des gcmalloc/gcrealloc) */
 	static int	_collect_freq_maybe;    /* fréquence de collection lors de l'appel à maybeCollect() */
	static int	_since_last_collection; /* nombre d'octets restants depuis la dernière collection */
 	static bool	_enable_auto;           /* collection automatique? */
	static bool	_enable_maybe;          /* collection lors des maybeCollect()? */
	static bool	_enable_collection;     /* collection autorisée? */
	static int	status;

 	enum { stat_collect, stat_finalize, stat_alloc, stat_broken };

#ifdef HAVE_PTHREAD
	static void	siggc_handler(int);
	static inline void  lock()   { threads->lock(); }
	static inline void  unlock() { threads->unlock(); }
#else
	static void	siggc_handler(int) { }
	static inline void  lock()   { }
	static inline void  unlock() { }
#endif

	static void         collect_unprotect();    /* interface pour la collection, verifie le enable_collect */
	static void         do_collect();           /* la collection proprement dite */

	static inline GCChunkNode *o2node(void *p)
        {
                return GCHash::get(p)->o2node(p, GCChunkNode::maskCollectable);
        }

	static inline size_t real_nbb(GCChunkNode *n) { return n->nbb() - sizeof(gc_header); }
public:
	static GCThread     *threads;        /* le gestionnaire de thread et de synchro */
         static void         (*internMemoryError)(unsigned int);

#ifdef HAVE_PTHREAD
        static inline void  unlock_dont_recovery() { threads->unlock_dont_recovery(); }
	static        void  die_if_sigsegv_occured_during_collection(void *addr);
#else
	static        void  die_if_sigsegv_occured_during_collection(void *addr) { }
#endif

       static void defaultMemoryError(unsigned int sz){
            unlock();
               internMemoryError(sz);
    lock();
  }

	static void	initialise(Collector::markerFn marker);
	static void	destroy();

	static int	siggc();

#ifdef HAVE_PTHREAD
	static void	        inject_my_thread(void *base_sp);
	static inline void	remove_thread(GCThreadCollector *loc) { threads->remove(loc); }
	static inline int       isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
		return threads->isStable(fct, a0, a1, a2, a3, a4, a5, a6, a7);
	}
#else
	static inline int       isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
		return 0;
	}
#endif

	static inline void  *allocate_unprotected(size_t sz) { return allocator->alloc(sz); }
	static inline void  free_unprotected(void *ptr) { allocator->free(ptr); }

	static inline gc *begOf(void *p) {
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
 		if(_enable_auto && (_since_last_collection <= (_collect_freq_auto - _collect_freq_maybe)))
 			collect(); 
 	}

 	static inline gc *gcmalloc(VirtualTable *vt, size_t n) {
 		lock();
 		register GCChunkNode *header = allocator->alloc_chunk(n + sizeof(gc_header), 1, current_mark & 1);
		
 		header->append(used_nodes);
		//printf("Allocate %d bytes at %p [%p] %d %d\n", n, header->chunk()->_2gc(), header, header->nbb(), real_nbb(header));
		
 		register struct gc_header *p = header->chunk();
 		p->_XXX_vt = vt;

 		_since_last_collection -= n;
 		if(_enable_auto && (_since_last_collection <= 0))
 			collect_unprotect();

 		unlock();
 		return p->_2gc();
 	}

 	static inline gc *gcrealloc(void *ptr, size_t n) {
 		lock();

 		GCPage    	*desc = GCHash::get(ptr);
 		GCChunkNode	*node = desc->o2node(ptr, GCChunkNode::maskCollectable);

		if(!node)
			gcfatal("%p isn't a avalid object", ptr);

 		size_t			old_sz = node->nbb();
 		GCChunkNode	*res = allocator->realloc_chunk(desc, node, n+sizeof(gc_header));

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

	static inline unsigned int enable(unsigned int n)  {
 		register unsigned int old = _enable_collection;
 		_enable_collection = n; 
 		return old;
 	}

 	static inline bool isMarked(GCChunkNode *node) { return node->mark() == (current_mark & 1); }
	static inline void mark(GCChunkNode *node) { node->_mark(current_mark & 1); }

 	static inline void trace(GCChunkNode *node) {
 		gc_header *o = node->chunk();
    o->_2gc()->tracer(real_nbb(node));
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

  static inline void applyFunc(void (*func)(gc *o, void *data), void *data){
    lock(); /* on s'assure que personne n'alloue ou ne collect */
    status = stat_collect; /* empeche les collections */
    GCChunkNode *cur=used_nodes->next(); /* reccupère l'amorce */
    unlock(); /* on peut allouer autant qu'on veut maintenant */ 

    for(; cur!=used_nodes; cur=cur->next())
      func(cur->chunk()->_2gc(), data);
    
    /* pas besoin de lock ! */
    status = stat_alloc; 
  }
};

}
#endif
