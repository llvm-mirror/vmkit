//===--------------- gcalloc.cc - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gcalloc.h"
#include "types.h"

GCChunkNode *GCAllocator::alloc_headers(uintptr_t headers_nbb, void *base,  
                                        uintptr_t depl, uintptr_t filled) {
	GCChunkNode *headers;
	uintptr_t rounded = GCMappedArea::round(headers_nbb);
	
	if(headers_nbb == rounded)
		headers = (GCChunkNode *)(new GCMappedArea(&headers_area, rounded))->area();
	else {
		headers = used_headers;
		used_headers = (GCChunkNode *)((uintptr_t)used_headers + headers_nbb);
		if(used_headers >= max_headers) {
			headers = (GCChunkNode *)(new GCMappedArea(&headers_area, used_headers_nbb))->area();
			used_headers = (GCChunkNode *)((uintptr_t)headers + headers_nbb);
			max_headers = (GCChunkNode *)((uintptr_t)headers + used_headers_nbb);
		}
	}

	register GCChunkNode *cur = headers;
	register GCChunkNode *lim = (GCChunkNode *)((uintptr_t)headers + headers_nbb);
	register GCChunkNode *max = lim - (1 + filled);
	for(;cur<max; cur++) {
		cur->initialise(cur+1, base);
		base = (void *)((uintptr_t)base + depl);
	}
	for(;cur<lim; cur++) {
 		cur->initialise(0, base);
		base = (void *)((uintptr_t)base + depl);
 	}
	return headers;
}

GCChunkNode	*GCAllocator::alloc_list(GCFreeList *fl, uintptr_t n) {
 	GCChunkNode	  *headers;
 	uintptr_t				area_nbb;
 	void          *area_ptr;
 	GCPage       	*page;

	if(GCFreeListFinder::isMmaped(n)) {
 		area_nbb = GCMappedArea::round(n);

 		/* on alloue un Descripteur */
 		page = new GCDescriptorMappedChunk(&normal_area, area_nbb, area_nbb);
  		area_ptr = page->area();

 		headers = page->headers();
  	} else {
  		uintptr_t chunk_nbb = fl->chunk_nbb();

			/* on alloue un Descripteur */
			page = new GCPage(&normal_area, area_nbb = fl->area_nbb(), chunk_nbb);

			area_ptr = page->area();

			/* on alloue des headers */
			headers = alloc_headers(fl->headers_nbb(), area_ptr, chunk_nbb, fl->filled());

  		page->headers(headers);
  	}

 	/* on hash notre nouveau chunk */
 	GCHash::hash_unprotected(page, area_ptr, area_nbb, area_nbb);
	
	fl->list(headers);
	return headers;
}

#define min(a, b)   ((a) < (b) ? (a) : (b))
#define max(a, b)   ((a) < (b) ? (b) : (a))

GCFreeListFinder::GCFreeListFinder() {
 	unsigned int	i, j, m;
 	GCFreeList		*cur;

 	for(i=0; i<=min_exp; i++)
 		exp_lists[i] = _exp_lists;
 	_exp_lists[0].initialise(min_exp, 128*min_exp);

 	for(i=min_exp_log, cur=_exp_lists+1; i<max_exp_log; i++, cur++) {
 		m = 1 << (i + 1);
 		cur->initialise(m, 512*m);
 		for(j=1<<i; j<m; j++)
 			exp_lists[j+1] = cur;
 	}

 	for(i=0;i<nb_lin; i++) {
 		m = ((i+1)<<lin_step_log) + max_exp;
 		lin_lists[i].initialise(m, min(128*m, 65536));
 	}

 	//	for(i=0; i<max_lin+2; i++)
 	//		printf("%d => %d\n", i, find(i)->chunk_nbb());
}


GCAllocator::GCAllocator() {
 	used_headers = 0;
 	max_headers = 0;
 	GCHash::initialise();
}

GCAllocator::~GCAllocator() {
 	GCMappedArea *cur;

 	for(cur=normal_area.next(); cur!=&normal_area; cur=cur->next())
 		delete cur->munmap();

 	for(cur=headers_area.next(); cur!=&headers_area; cur=cur->next())
 		delete cur->munmap();

 	for(cur=mapped_area.next(); cur!=&mapped_area; cur=cur->next())
 		delete cur->munmap();
	GCHash::unlink();
}

void *GCAllocator::operator new(uintptr_t req) {
	uintptr_t nbb = GCMappedArea::round(req);
	GCAllocator *res = (GCAllocator *)GCMappedArea::do_mmap(nbb);
	res->my_size = nbb;
	return res;
}

void GCAllocator::operator delete(void *ptr) {
	GCMappedArea::do_munmap(ptr, ((GCAllocator *)ptr)->my_size);
}
