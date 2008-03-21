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

class GCMinAlloc;

class GCMin {
public:
	inline GCMin() {}
	inline ~GCMin() {}
	inline void *operator new(size_t);
	inline void operator delete(void *, size_t);
};

class GCMappedArea : public GCMin {
 	GCMappedArea	*_prev;
 	GCMappedArea	*_next;
 	void					*_mapped_area;
 	size_t				_mapped_nbb;

public:
 	static size_t round(size_t n) { return (n + PAGE_SIZE - 1) & -PAGE_SIZE; }

 	GCMappedArea	*prev() { return _prev; }
 	GCMappedArea	*next() { return _next; }
	
 	inline GCMappedArea() { _prev = _next = this; }
 	inline GCMappedArea(size_t n) { _prev = _next = this; mmap(n); }
 	inline GCMappedArea(GCMappedArea *p, size_t n) { append(p); mmap(n); }

 	inline void		initialise(void *p, size_t n) { _mapped_area = p; _mapped_nbb = n; }
 	inline size_t nbb() const { return _mapped_nbb; }
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
 	GCMappedArea *	mmap(size_t sz);
 	int							mremap(size_t sz);
	
 	static void *do_mmap(size_t sz);
 	static void  do_munmap(void *, size_t sz);
};

class GCMinAllocStack {
 	unsigned int		current;
 	unsigned int		max;
 	unsigned int		*free_list;
 	unsigned short	nbb;
	
public:
 	inline void initialise(size_t n) { nbb = n; current = max = 0; free_list = 0; }

	inline void fill(unsigned int st, size_t n) {
		current = st;
		max = st + n;
	}
	
 	inline void inject_free(void *ptr) {
 		*((unsigned int **)ptr) = free_list;
 		free_list = (unsigned int *)ptr;
 	}

 	inline void *alloc() {
 		register unsigned int res;
 		if(free_list) {
 			res = (unsigned int)free_list;
 			free_list = *((unsigned int **)res);
 		} else {
 			res = current;
 			current += nbb;
 			if(current > max)
 				return 0;
 		}
		
 		return (void *)res;
 	}
};

class GCMinAlloc {
 	static const size_t	log_max_min_alloc = 6;											/* 64 octets */
 	static const size_t	max_min_alloc = (1 << log_max_min_alloc);
 	static const size_t log_step = 2;
	
 	/* piles d'allocations */
 	static GCMinAllocStack			_stacks[max_min_alloc >> log_step];
 	static GCMinAllocStack			*stacks[max_min_alloc+1];

	static GCMappedArea         base_area;

 	static void *alloc_area(GCMinAllocStack *s);

	inline GCMinAlloc() {} /* pas d'instance de ce truc */
public:
	static void initialise();
	static void destroy();
	
 	static inline void *minalloc(size_t);
 	static inline void minfree(void *, size_t);
};

inline void *GCMinAlloc::minalloc(size_t nbb) {
 	GCMinAllocStack *s = stacks[nbb];
 	void *res = s->alloc();

 	if(!res)
 		res = alloc_area(s);
 	return res;
}

inline void GCMinAlloc::minfree(void *ptr, size_t nbb) {
 	stacks[nbb]->inject_free(ptr);
}

inline void *GCMin::operator new(size_t nbb) { 
 	return GCMinAlloc::minalloc(nbb);
}

inline void GCMin::operator delete(void *ptr, size_t nbb) { 
	GCMinAlloc::minfree(ptr, nbb);
}



#endif
