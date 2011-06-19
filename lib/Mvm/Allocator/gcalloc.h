//===---------------- gcalloc.h - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_ALLOC_H_
#define _GC_ALLOC_H_

#include <string.h> /* memset */

#include "gcchunk.h"
#include "types.h"

class GCFreeList {
 	GCChunkNode 	 *_list;
 	uintptr_t					_chunk_nbb;
 	uintptr_t					_area_nbb;
 	uintptr_t					_nb_chunks;
 	uintptr_t					_headers_nbb;
 	uintptr_t          _filled;
public:
 	inline GCFreeList() { _chunk_nbb = 0; _nb_chunks = 1; }

 	inline void			initialise(uintptr_t n, uintptr_t area) {
 		_list					= 0;
 		_chunk_nbb		= n; 
 		_area_nbb			= GCMappedArea::round(area); /* taille en pages utilisée pour une free-list */
 		_nb_chunks		= _area_nbb/_chunk_nbb;      /* nombre de type que je peux caser la dedans  */
 		/* est-ce qu'il reste de la place derière ? */
 		_filled = (_nb_chunks * _chunk_nbb) == _area_nbb ? 0 : 1;
 		/* j'utilise un "faux header" à 0 pour completer ce truc */
 		_headers_nbb	= (_nb_chunks + _filled)*sizeof(GCChunkNode);
 	}

 	inline GCChunkNode    *list() { return _list; }
 	inline void						list(GCChunkNode *l) { _list = l; }
 	inline uintptr_t					nb_chunks()		{ return _nb_chunks; }
 	inline uintptr_t					chunk_nbb()		{ return _chunk_nbb; }
 	inline uintptr_t					area_nbb()		{ return _area_nbb; }
 	inline uintptr_t					headers_nbb() { return _headers_nbb; }
 	inline uintptr_t         filled()      { return _filled; }

 	inline void reject(GCChunkNode *header) {
 		header->next(_list);
 		_list = header;
 	}
};

class GCFreeListFinder {
 	/* trois zones distinctes: exponentielle/lineaire/mmap */
 	static const unsigned int min_exp_log = 0;						/* 0  -> 4 => 4 */
 	static const unsigned int max_exp_log = 5;						/* 4  -> 32 en exponentielle */
 	static const unsigned int max_lin_log = 13;						/* 32 -> 128 en linéaire */
 	static const unsigned int lin_step_log = 4;						/* par pas de 4 */
 	static const unsigned int min_exp = 1 << min_exp_log;
 	static const unsigned int max_exp = 1 << max_exp_log;
 	static const unsigned int max_lin = 1 << max_lin_log;
 	static const unsigned int	nb_exp = max_exp_log - min_exp_log + 1;
 	static const unsigned int nb_lin = (max_lin - max_exp)>>lin_step_log;

 	GCFreeList		_exp_lists[nb_exp];
 	GCFreeList		*exp_lists[max_exp+1];
 	GCFreeList		lin_lists[nb_lin];
 	GCFreeList		mmap_list;

 	inline uintptr_t linEntry(uintptr_t n) { return (n - max_exp - 1)>>lin_step_log; }
public:
 	GCFreeListFinder();

 	static inline bool		isMmaped(uintptr_t n) { return n>max_lin; }

 	inline GCFreeList *find(uintptr_t n) {
 		return isMmaped(n) ? &mmap_list : (n>max_exp) ? lin_lists + linEntry(n) : exp_lists[n]; 
 	}
};

class GCAllocator {
 	static const unsigned int		used_headers_nbb = (PAGE_SIZE + (4096*sizeof(GCChunkNode)) - 1) & -PAGE_SIZE;

	uintptr_t    my_size;
 	GCFreeListFinder	undefined_finder;
 	GCFreeListFinder	zero_filled_finder;

	GCMappedArea		normal_area;    /* area pour les petit chunk */
	GCMappedArea		mapped_area;    /* pour les gros */
	GCMappedArea		headers_area;   /* liste des mmap des headers */
	GCChunkNode		  *used_headers;  /* une liste de headers pour les prochaines alloc_list */
	GCChunkNode		  *max_headers;   /* la limite */

	GCChunkNode     *alloc_headers(uintptr_t, void *, uintptr_t, uintptr_t);
	GCChunkNode     *alloc_list(GCFreeList *, uintptr_t);
public:
	GCAllocator();
 	~GCAllocator();

	void *operator new(uintptr_t);
	void operator delete(void *);

 	inline GCPage        *o2page(void *ptr) { return GCHash::get(ptr); }

 	inline GCChunkNode *alloc_chunk(uintptr_t n, bool isCol, unsigned int m) {
		GCFreeList			     *fl = undefined_finder.find(n);
		register GCChunkNode *res = fl->list();
		
		if(!res)
			res = alloc_list(fl, n);
		
		fl->list(res->next());
		res->nbb(n, isCol, m);

		return res;
 	}
	
 	inline void reject_chunk(GCPage *page, GCChunkNode *header) {
 		GCFreeList *fl = undefined_finder.find(page->chunk_nbb());
		
 		if(fl->chunk_nbb()) {
			//			printf("0 Filled %p with %d bytes\n", header->chunk(), header->nbb());
			memset((void*)header->chunk(), 0, header->nbb());
			header->free();
 			fl->reject(header);
		} else {
			GCHash::hash_unprotected(page, page->area(), page->nbb(), 0);
			page->reject();
			delete (GCPage *)(page->munmap());
 		}
 	}
	
 	inline void reject_chunk(GCChunkNode *header) {
 		reject_chunk(o2page(header->chunk()), header);
 	}
	
 	inline GCChunkNode *stupid_realloc_chunk(GCChunkNode *old_header, uintptr_t new_nbb) {
 		GCChunkNode *new_header = alloc_chunk(new_nbb, old_header->isCollectable(), old_header->mark());
 		uintptr_t old = old_header->nbb();
 		uintptr_t nbb = old < new_nbb ? old : new_nbb;
 		memcpy((void*)new_header->chunk(), (void*)old_header->chunk(), nbb);
 		return new_header;
 	}
	
 	inline GCChunkNode *realloc_chunk(GCPage *page, GCChunkNode *old_header, uintptr_t new_nbb) {
 		GCChunkNode *new_header = old_header;
 		uintptr_t	     max_nbb = page->chunk_nbb();
		
 		if(GCFreeListFinder::isMmaped(max_nbb))
 			if(GCFreeListFinder::isMmaped(new_nbb)) {
 				/* le plus compliqué, on essaye de faire un mremap d'un petit bout... */
 				uintptr_t							rounded = GCMappedArea::round(new_nbb);
				uintptr_t              old_nbb = page->nbb();
 				signed int					depl = rounded - page->nbb();
 				if(depl) {
 					if(page->mremap(rounded) == -1) /* perdu */
 						return new_header = stupid_realloc_chunk(old_header, new_nbb);
 					else {
 						if(depl > 0)
 							GCHash::hash_unprotected(page, (void *)((uintptr_t)page->area() + old_nbb), depl, depl);
 						else
 							GCHash::hash_unprotected(page, (void *)((uintptr_t)page->area() + rounded), -depl, 0);
 					}
 				}
				page->chunk_nbb(rounded);
 				old_header->nbb(new_nbb);
 			} else
 				new_header = stupid_realloc_chunk(old_header, new_nbb);
 		else
 			if(new_nbb <= max_nbb)
 				old_header->nbb(new_nbb);
 			else
 				new_header = stupid_realloc_chunk(old_header, new_nbb);
 		return new_header;
 	}

 	inline void *alloc(uintptr_t sz) { return alloc_chunk(sz, 0, 0)->chunk(); }
	
 	inline void free(void *ptr) {
 		GCPage     	*page = o2page(ptr);
 		GCChunkNode	*header = page->o2node(ptr, GCChunkNode::maskNotCollectable);

 		if(!header)
 			gcfatal("%p isn't an object", ptr);
		
 		reject_chunk(page, header);
 	}

 	inline void *realloc(void *ptr, uintptr_t nbb) {
 		GCPage      	*page = o2page(ptr);
 		GCChunkNode   *header = page->o2node(ptr, GCChunkNode::maskNotCollectable);
		
 		if(!header)
 			gcfatal("%p isn't an object", header);
		
 		register GCChunkNode *new_header = realloc_chunk(page, header, nbb);
 		if(new_header != header)
 			reject_chunk(page, header);

 		return new_header->chunk();
 	}
};

#endif
