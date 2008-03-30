//===--------------- gcmapper.h - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_MAPPER_H_
#define _GC_MAPPER_H_

#include "gcerror.h"
#include "osdep.h"
#include "types.h"

class GCMinAlloc;

class GCMin {
public:
	inline GCMin() {}
	inline ~GCMin() {}
	inline void *operator new(uintptr_t);
	inline void operator delete(void *, uintptr_t);
};

class GCMappedArea : public GCMin {
 	GCMappedArea	*_prev;
 	GCMappedArea	*_next;
 	void					*_mapped_area;
 	uintptr_t				_mapped_nbb;

public:
 	static uintptr_t round(uintptr_t n) { return (n + PAGE_SIZE - 1) & -PAGE_SIZE; }

 	GCMappedArea	*prev() { return _prev; }
 	GCMappedArea	*next() { return _next; }
	
 	inline GCMappedArea() { _prev = _next = this; }
 	inline explicit GCMappedArea(uintptr_t n) { _prev = _next = this; mmap(n); }
 	inline GCMappedArea(GCMappedArea *p, uintptr_t n) { append(p); mmap(n); }

 	inline void		initialise(void *p, uintptr_t n) { _mapped_area = p; _mapped_nbb = n; }
 	inline uintptr_t nbb() const { return _mapped_nbb; }
 	inline void		*area() { return _mapped_area; }

 	inline void append(GCMappedArea *p) {
 		_prev = p;
 		_next = p->_next;
 		p->_next = this;
 		_next->_prev = this;
 	}

 	inline void reject() {
 		_prev->_next = _next;
 		_next->_prev = _prev;
 	}
	
 	GCMappedArea *	munmap();
 	GCMappedArea *	mmap(uintptr_t sz);
 	int							mremap(uintptr_t sz);
	
 	static void *do_mmap(uintptr_t sz);
 	static void  do_munmap(void *, uintptr_t sz);
};

class GCMinAllocStack {
 	uintptr_t		current;
 	unsigned int		max;
 	unsigned int		*free_list;
 	unsigned short	nbb;
	
public:
 	inline void initialise(uintptr_t n) { nbb = n; current = max = 0; free_list = 0; }

	inline void fill(unsigned int st, uintptr_t n) {
		current = st;
		max = st + n;
	}
	
 	inline void inject_free(void *ptr) {
 		*((unsigned int **)ptr) = free_list;
 		free_list = (unsigned int *)ptr;
 	}

 	inline void *alloc() {
 		register uintptr_t res;
 		if(free_list) {
 			res = (uintptr_t)free_list;
 			free_list = *((unsigned int **)res);
 		} else {
 			res = current;
 			current += nbb;
 			if(current > max)
 				return 0;
 		}
		
 		return (void*)res;
 	}
};

class GCMinAlloc {
 	static const uintptr_t	log_max_min_alloc = 6;											/* 64 octets */
 	static const uintptr_t	max_min_alloc = (1 << log_max_min_alloc);
 	static const uintptr_t log_step = 2;
	
 	/* piles d'allocations */
 	static GCMinAllocStack			_stacks[max_min_alloc >> log_step];
 	static GCMinAllocStack			*stacks[max_min_alloc+1];

	static GCMappedArea         base_area;

 	static void *alloc_area(GCMinAllocStack *s);

	inline GCMinAlloc() {} /* pas d'instance de ce truc */
public:
	static void initialise();
	static void destroy();
	
 	static inline void *minalloc(uintptr_t);
 	static inline void minfree(void *, uintptr_t);
};

inline void *GCMinAlloc::minalloc(uintptr_t nbb) {
 	GCMinAllocStack *s = stacks[nbb];
 	void *res = s->alloc();

 	if(!res)
 		res = alloc_area(s);
 	return res;
}

inline void GCMinAlloc::minfree(void *ptr, uintptr_t nbb) {
 	stacks[nbb]->inject_free(ptr);
}

inline void *GCMin::operator new(uintptr_t nbb) { 
 	return GCMinAlloc::minalloc(nbb);
}

inline void GCMin::operator delete(void *ptr, uintptr_t nbb) { 
	GCMinAlloc::minfree(ptr, nbb);
}



#endif
