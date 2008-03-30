//===---------------- gcchunk.cc - Mvm allocator --------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gcchunk.h"


GCHashSet		  	*GCHash::sets[GCHashConst::nb_set_per_hash];
unsigned int  	GCHash::used[GCHashConst::nb_set_per_hash];
GCHashSet       GCHash::empty;
bool            GCHash::inited = 0;
size_t          GCHash::nb_link = 0;

GCChunkNode     GCPage::empty;

GCPage          GCHashSet::empty;

void GCHashSet::hash(GCPage *d, void *base, size_t nbb, size_t nbb_map, unsigned int *c) {
 	register unsigned int entry = GCHashConst::desc_entry(base);
 	register unsigned int pbase = entry << PAGE_SHIFT;
 	register unsigned int top_del = (pbase + nbb) >> PAGE_SHIFT;
 	register unsigned int top_add = (pbase + nbb_map) >> PAGE_SHIFT;

	//	printf("   %p Hash from %d to %d and to %d\n", this, entry, top_add, top_del);
 	for(; entry<top_add; entry++) {
 		pages[entry] = d;
 		(*c)++;
 	}
 	for(; entry<top_del; entry++) {
 		pages[entry] = &empty;
 		(*c)--;
 	}
}

void GCHash::hash_unprotected(GCPage *desc, void *base, unsigned int nbb, unsigned int nbb_map) {
 	uintptr_t	entry = GCHashConst::set_entry(base);
 	uintptr_t	cur_sz = GCHashConst::set_entry_2_ptr(entry + 1) - (uintptr_t)base;	/* taille restante */
 	cur_sz = ((cur_sz < nbb) ? cur_sz : nbb);													/* on prends le min avec nbb   */
	
	//	printf("Hash %p (%p %p %d) in entry %d with %d/%d\n", desc, desc->area(), base, desc->nbb(), entry, cur_sz, nbb_map);
	while(cur_sz) {
		if(sets[entry] == &empty)
			sets[entry] = new GCHashSet();
		
		sets[entry]->hash(desc, base, cur_sz, (nbb_map > cur_sz) ? cur_sz : nbb_map, used + entry);
		
		if(!used[entry]) {
			delete sets[entry];
			sets[entry] = &empty;
		}
		entry++;
		base = (void *)((uintptr_t)base + cur_sz);
		nbb -= cur_sz;
		nbb_map = (nbb_map < cur_sz) ? 0 : nbb_map - cur_sz;
		cur_sz = (nbb < GCHashConst::set_nbb) ? nbb : GCHashConst::set_nbb;
	}
}

void GCPage::initialise() {
 	empty.initialise(0, 0);
}

GCHashSet::GCHashSet() {
 	unsigned int i;
	
 	for(i=0; i<GCHashConst::nb_desc_per_set; i++)
 		pages[i] = &empty;
}

void GCHash::initialise() {
	if(!inited) {
		inited = 1;
		GCMinAlloc::initialise();
		
		unsigned int i;
		
		for(i=0; i<GCHashConst::nb_set_per_hash; i++) {
			sets[i] = &empty;
			used[i] = 0;
		}
		GCPage::initialise();
	}
	nb_link++;
}

void GCHash::destroy() {
	if(inited) {
		if(nb_link)
			gcwarning2("Can't destroy GC hash map: you have clients connected on it\n");
		else {
			inited = 0;
			unsigned int i;
	
			for(i=0; i<GCHashConst::nb_set_per_hash; i++)
				if(sets[i] != &empty)
					delete sets[i];

			GCMinAlloc::destroy();
		}
	}
}

void *GCHashSet::operator new(size_t sz) {
	return (void *)GCMappedArea::do_mmap(sz);
}

void GCHashSet::operator delete(void *ptr, size_t sz) {
	GCMappedArea::do_munmap(ptr, sz);
}
