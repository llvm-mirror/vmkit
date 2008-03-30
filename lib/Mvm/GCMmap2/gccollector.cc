//===----------- gccollector.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gccollector.h"
#include <setjmp.h>

using namespace mvm;

void GCCollector::do_collect() {
	//printf("----- do collect -----\n");
	jmp_buf   buf;
	setjmp(buf);
 	GCChunkNode	*cur;
 	_since_last_collection = _collect_freq_auto;

 	current_mark++;

 	unused_nodes->attrape(used_nodes);

#ifdef HAVE_PTHREAD
	threads->synchronize();
#endif

 	for(cur=used_nodes->next(); cur!=used_nodes; cur=cur->next())
 		trace(cur);

 	if(_marker)
 		_marker();

 	status = stat_finalize;

 	/* finalize */
 	GCChunkNode	finalizable;
 	finalizable.attrape(unused_nodes);

 	status = stat_alloc;
#ifdef HAVE_PTHREAD
	threads->collectionFinished();
#endif
	
 	/* kill everyone */
 	GCChunkNode *next;

 	for(cur=finalizable.next(); cur!=&finalizable; cur=next) {
 		register gc_header *c = cur->chunk();
 		next = cur->next();
		
 		c->_2gc()->destroyer(real_nbb(cur));
		
		//printf("    !!!! reject %p [%p]\n", cur->chunk()->_2gc(), cur);
 		allocator->reject_chunk(cur);
 	}

}

void GCCollector::collect_unprotect() {
 	if(_enable_collection && (status == stat_alloc)) {
 		status = stat_collect;
 		do_collect();
 	}
}

#ifdef HAVE_PTHREAD
void GCCollector::die_if_sigsegv_occured_during_collection(void *addr) {
	if(!isStable(0, 0, 0, 0, 0, 0, 0, 0, 0)) {
		printf("; ****************************************************** ;\n");
		printf(";         SIGSEGV occured during a collection            ;\n");
		printf(";   I'm trying to let the allocator in a coherent stat   ;\n");
		printf("; but the collector is DEAD and will never collect again ;\n");
		printf("; ****************************************************** ;\n");
		
		status = stat_broken;                 /* Collection is finished and no other collection will happend */
		threads->cancel();                    /* Emulates a full collection to unlock mutators */
		used_nodes->eat(unused_nodes);        /* All nodes are uses. Finalized are lost */
		unlock_dont_recovery();               /* Unlocks the GC lock */
		//gcfatal("SIGSEGV occured during collection at %p", addr);
	}
}
#endif /* HAVE_PTHREAD */

void GCCollector::gcStats(size_t *_no, size_t *_nbb) {
 	register unsigned int n, tot;
 	register GCChunkNode *cur;
 	lock();
 	for(n=0, tot=0, cur=used_nodes->next(); cur!=used_nodes; cur=cur->next(), n++)
 		tot += cur->nbb();
 	unlock();
 	*_no = n;
 	*_nbb = tot;
}

