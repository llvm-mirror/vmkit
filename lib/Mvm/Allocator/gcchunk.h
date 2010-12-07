//===---------------- gcchunk.h - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_CHUNK_H_
#define _GC_CHUNK_H_

//#include "mvm/VirtualMachine.h"
//#include "mvm/GC.h"

#include "gcmapper.h"
#include "types.h"

namespace mvm {
	class gcRoot;
}

using namespace mvm;

/* description d'un bout de mémoire */
class GCChunkNode {
 	GCChunkNode *_prev;     /* bit 0-1: l'age, les autres: le previous */
 	GCChunkNode	*_next;
 	void *       _chunk;
 	uintptr_t		 _nbb_mark;	/* nbb = 0 <=> ce chunk est libre */
	                        /* bit 0-2: la marque */
	                        /* bit 3: est-on collectable */
public:
 	static const signed int maskCollectable    = 8;
 	static const signed int maskNotCollectable = 0;

 	inline GCChunkNode() {}
 	inline ~GCChunkNode() {}

 	inline gcRoot	     *	chunk()                { return (gcRoot*)_chunk; }

 	inline GCChunkNode *	next()                 { return _next; }
 	inline void						next(GCChunkNode *n)   { _next = n; }

 	inline GCChunkNode *	prev()                 { return _prev; }
 	inline GCChunkNode *  prev(GCChunkNode *p)   { return _prev = p; }

 	inline void						free() { _nbb_mark = 0; }

 	inline void						nbb(uintptr_t n, bool isCol, int m) { _nbb_mark = (n << 4) | ((isCol & 0x1) << 3) | m; }
 	inline void						nbb(uintptr_t n)					        	 { _nbb_mark = (n << 4) | (_nbb_mark & 0xf); }
 	inline uintptr_t					nbb()                            { return _nbb_mark >> 4; }

	/* pas de verification de debordement!!!! */
 	inline void						_mark(unsigned int m)             { _nbb_mark = m | (_nbb_mark & -8); }
 	inline unsigned int		mark()                           { return _nbb_mark & 7; }

 	inline signed int      isCollectable() { return _nbb_mark & 8; }

 	inline void initialise(GCChunkNode *n, void *c) {
 		_chunk = c;
 		_next = n;
 		_nbb_mark = 0;
 	}

	/* pour faire une amorce de liste */
 	inline void alone() { _prev = _next = this; }

	/* ajoute this à p */
 	inline void append(GCChunkNode *p) {
 		_prev = p;
 		_next = p->_next;
 		p->_next = this;
 		_next->_prev = this;
 	}
	
  /* ajoute this à p */
 	inline void prepend(GCChunkNode *p) {
 		_next = p;
    _prev = p->_prev;
    p->_prev = this;
    _prev->_next = this;
 	}

	/* enleve this de sa liste */
 	inline void remove() {
 		_next->_prev = _prev;
 		_prev->_next = _next;
 	}

	/* place la liste de p dans this. Ecrase celle de p */
	inline void attrape(GCChunkNode *l)	{
		if(l == l->_next)
			alone();
		else {
			(_next = l->_next)->_prev = this;
			(_prev = l->_prev)->_next = this;
			l->alone();
		}
	}

	/* ajoute la liste de l dans this */
 	inline void eat(GCChunkNode *l) {
 		if(l != l->_next) {
 			(_next->_prev = l->_prev)->_next = _next;
 			(_next = l->_next)->_prev = this;
 			l->alone();
 		}
 	}
};

/* entete de description de zone mémoire */
/* une zone est un ensemble de bouts de mémoire de même taille, contigues, aligné sur des pages */
class GCPage : public GCMappedArea {
 	static GCChunkNode empty;

 	GCChunkNode	*_headers;
 	uintptr_t			 _chunk_nbb;
public:
 	static void initialise();

 	inline GCPage() {
 		GCMappedArea::initialise(0, 0);
 		_headers = &empty;
 		_chunk_nbb = ((unsigned int)-1) >> 1;		/* on évite le /0 */
 	}

 	inline GCPage(GCMappedArea *p, uintptr_t mapped_nbb, uintptr_t cnbb)
 		: GCMappedArea(p, mapped_nbb) {
 		_chunk_nbb = cnbb;
 	}

 	inline ~GCPage() {}     /* FAIRE LE UNMAP A LA MAIN!!!!! */

 	inline uintptr_t					chunk_nbb()		        { return _chunk_nbb; }
 	inline void 					chunk_nbb(uintptr_t n)		{ _chunk_nbb = n; }
 	inline GCChunkNode   *headers()			        { return _headers; }
 	inline void headers(GCChunkNode *h)         { _headers = h;; }

 	inline GCChunkNode *o2node(void *ptr, signed int mask) {
 		register uintptr_t entry = ((uintptr_t)ptr - (uintptr_t)area())/_chunk_nbb;
 		register GCChunkNode *res = _headers + entry;
 		return ((uintptr_t)ptr - (uintptr_t)res->chunk() < res->nbb())
 			&& (res->isCollectable() == mask)
 			? res : 0;
 	}
	
 	inline gcRoot *o2header(void *ptr, signed int mask) {
 		register uintptr_t entry = ((uintptr_t)ptr - (uintptr_t)area())/_chunk_nbb;
 		register GCChunkNode *res = _headers + entry;
 		return ((uintptr_t)ptr - (uintptr_t)res->chunk() < res->nbb())
 			&& (res->isCollectable() == mask)
 			? res->chunk() : 0;
 	}
};

class GCDescriptorMappedChunk : public GCPage {
	GCChunkNode	header;
public:
	inline GCDescriptorMappedChunk(GCMappedArea *p, uintptr_t mapped_nbb, uintptr_t cnbb) 
		: GCPage(p, mapped_nbb, cnbb) {
		header.initialise(0, area());
		headers(&header);
	}
};

class GCHashConst {
public:
 	static const unsigned int		desc_bits = PAGE_SHIFT;
 	static const unsigned int		set_bits = 10;
 	static const unsigned int		hash_bits = (32 - set_bits - desc_bits);
 	static const unsigned int		nb_desc_per_set = 1 << set_bits;
 	static const unsigned int		nb_set_per_hash = 1 << hash_bits;
 	static const unsigned int		set_nbb = 1 << (set_bits + desc_bits);

 	static uintptr_t	desc_entry(void * ptr) { return ((uintptr_t)ptr >> desc_bits) & (( 1 << set_bits) - 1); }
 	static uintptr_t	set_entry(void *ptr) { return (uintptr_t)ptr >> (set_bits + desc_bits); }
 	static uintptr_t	set_entry_2_ptr(uintptr_t entry) { return entry << (set_bits + desc_bits); }
};

class GCHashSet {
 	static GCPage empty;

 	GCPage		*pages[GCHashConst::nb_desc_per_set];
public:
 	void *operator new(uintptr_t);
 	void operator delete(void *, uintptr_t);
	
 	GCHashSet();

 	void hash(GCPage *, void *, uintptr_t, uintptr_t, unsigned int *);

 	inline GCPage *get(void *ptr) { return pages[GCHashConst::desc_entry(ptr)]; }
};

class GCHash {
 	static GCHashSet			*sets[GCHashConst::nb_set_per_hash];
 	static unsigned int		used[GCHashConst::nb_set_per_hash];
 	static GCHashSet	    empty;
	static bool           inited;
	static uintptr_t         nb_link;

public:
	static void unlink() { nb_link--; }
 	static void initialise();
 	static void destroy();

 	static void hash_unprotected(GCPage *p, void *base, unsigned int nbb, unsigned int nbb_map);
 	//void hash(GCPage *p) { hash(p, p->area(), p->nbb(), p->nbb()); }
 	//void unhash(GCPage *p) { hash(p, p->area(), p->nbb(), 0); }

 	static inline GCPage *get(void *ptr) { return sets[GCHashConst::set_entry(ptr)]->get(ptr); }
};

#endif
