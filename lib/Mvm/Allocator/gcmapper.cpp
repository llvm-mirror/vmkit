//===--------------- gcmapper.cc - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gcmapper.h"
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


GCMinAllocStack GCMinAlloc::_stacks[GCMinAlloc::max_min_alloc >> log_step];
GCMinAllocStack *GCMinAlloc::stacks[GCMinAlloc::max_min_alloc+1];
GCMappedArea    GCMinAlloc::base_area;



#if defined(__MACH__)
#define DO_MMAP(sz)					mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)
#else
#define DO_MMAP(sz)					mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)
#endif

#define DO_MUNMAP(ptr, sz)	munmap(ptr, sz)

#if defined(__MACH__)
static inline void* manual_mremap(void * addr, int old_size, int new_size, int flags)
{
  void * res = addr;
  if (new_size < old_size)
    DO_MUNMAP((void*)((int)addr + new_size), old_size - new_size);
  else if (new_size > old_size)
    // Use of MAP_FIXED is discouraged...
    // res = mmap(addr, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, MAP_FIXED);
    res=(void*)-1;
  
  return res;
}
#endif

static inline void *do_mmap(size_t sz) {
	void *res = DO_MMAP(sz);
	//printf("mmap %d bytes at %d\n", sz, res);
	if(res == MAP_FAILED)
		if (errno == ENOMEM)
      (*onMemoryError)(sz);
    else
      {
        gcfatal("unable to mmap %d bytes", sz);
      }
	return res;
}

static inline void do_munmap(void *ptr, size_t sz) {
	//	printf("munmap %d bytes at %p\n", sz, ptr);
	DO_MUNMAP(ptr, sz);
}

void *GCMappedArea::do_mmap(size_t sz) { return ::do_mmap(sz); }
void GCMappedArea::do_munmap(void *ptr, size_t sz) { ::do_munmap(ptr, sz); }

GCMappedArea *GCMappedArea::munmap() {
	::do_munmap(_mapped_area, _mapped_nbb);
	return this;
}

GCMappedArea *GCMappedArea::mmap(size_t n) {
	_mapped_area = ::do_mmap(_mapped_nbb = n);
	return this;
}

int GCMappedArea::mremap(size_t n) {
#if defined(__MACH__)
	void *res = manual_mremap(_mapped_area, _mapped_nbb, n, 0);
#else
	void *res = ::mremap(_mapped_area, _mapped_nbb, n, 0);
#endif
  
	if((intptr_t)res == -1)
		return -1;
	_mapped_area = res;
	_mapped_nbb = n;
	return 0;
}

void *GCMinAlloc::alloc_area(GCMinAllocStack *s) {
	GCMinAllocStack	*area_stack = stacks[sizeof(GCMappedArea)];
	GCMappedArea		*area = (GCMappedArea *)area_stack->alloc();

	if(!area) {
		/* pbm : on n'a vraiment plus rien :) */
		area = (GCMappedArea *)::do_mmap(PAGE_SIZE);
		area->initialise(area, PAGE_SIZE);
		area_stack->fill((uintptr_t)(area + 1), PAGE_SIZE - sizeof(GCMappedArea));
		area->append(&base_area);
		area = (GCMappedArea *)area_stack->alloc();
	}
	area->mmap(PAGE_SIZE);
	area->append(&base_area);
	s->fill((uintptr_t)area->area(), area->nbb());

	return s->alloc();
}

void GCMinAlloc::initialise() {
	unsigned int i, j, m;
	size_t nb_stacks = max_min_alloc >> log_step;
	
	stacks[0] = _stacks;
	for(i=0; i<nb_stacks; i++) {
		m = i<<log_step;
		_stacks[i].initialise(m + (1<<log_step));
		for(j=0; j<(1<<log_step); j++)
			stacks[m+j+1] = _stacks + i;
	}
}

void GCMinAlloc::destroy() {
 	unsigned int nb_area, i;
 	GCMappedArea *cur;

 	for(nb_area=0, cur=base_area.next(); cur!=&base_area; cur=cur->next(), nb_area++);

 	GCMappedArea* areas = (GCMappedArea*)alloca(sizeof(GCMappedArea) * nb_area);

 	for(i=0, cur=base_area.next(); cur!=&base_area; cur=cur->next(), i++)
 		areas[i] = *cur;

 	for(i=0; i<nb_area; i++)
 		areas[i].munmap();
}
