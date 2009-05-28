//===----------- gccollector.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gccollector.h"

using namespace mvm;

GCAllocator   *GCCollector::allocator = 0;
#ifdef HAVE_PTHREAD
GCThread      *GCCollector::threads;
#endif

GCCollector::markerFn   GCCollector::_marker;


int           GCCollector::status;

GCChunkNode    *GCCollector::used_nodes;
GCChunkNode    *GCCollector::unused_nodes;

unsigned int   GCCollector::current_mark;

int  GCCollector::_collect_freq_auto;
int  GCCollector::_collect_freq_maybe;
int  GCCollector::_since_last_collection;

bool GCCollector::_enable_auto;
bool GCCollector::_enable_maybe;
bool GCCollector::_enable_collection;

typedef void (*destructor_t)(void*);

void GCCollector::do_collect() {
  //printf("----- do collect -----\n");
  GCChunkNode  *cur;
#ifdef SERVICE
  mvm::Thread::get()->MyVM->_since_last_collection = _collect_freq_auto;
#else
  _since_last_collection = _collect_freq_auto;
#endif

  current_mark++;

  unused_nodes->attrape(used_nodes);

  mvm::Thread* th = th->get();
  th->MyVM->startCollection();

#ifdef HAVE_PTHREAD
  threads->synchronize();
#endif

  mvm::Thread* tcur = th;

  // (1) Trace the VM.
  th->MyVM->tracer();

  // (2) Trace the threads.
  do {
    tcur->tracer();
    tcur = (mvm::Thread*)tcur->next();
  } while (tcur != th);

  // (3) Trace stack objects.
  for(cur=used_nodes->next(); cur!=used_nodes; cur=cur->next())
    trace(cur);

  // (4) Trace the weak reference queue.
  th->MyVM->scanWeakReferencesQueue();

  // (5) Trace the soft reference queue.
  th->MyVM->scanSoftReferencesQueue();
  
  // (6) Trace the finalization queue.
  th->MyVM->scanFinalizationQueue();

  // (7) Trace the phantom reference queue.
  th->MyVM->scanPhantomReferencesQueue();

  if(_marker)
    _marker(0);
  status = stat_finalize;

  /* finalize */
  GCChunkNode  finalizable;
  finalizable.attrape(unused_nodes);

  status = stat_alloc;
  
  /* kill everyone */
  GCChunkNode *next = 0;
  for(cur=finalizable.next(); cur!=&finalizable; cur=next) {
    //printf("    !!!! reject %p [%p]\n", cur->chunk()->_2gc(), cur);
    next = cur->next();
    allocator->reject_chunk(cur);
  }


  th->MyVM->endCollection();
#ifdef HAVE_PTHREAD
  threads->collectionFinished();
#endif
  th->MyVM->wakeUpFinalizers();
  th->MyVM->wakeUpEnqueue();
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

