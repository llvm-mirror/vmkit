//===----------- gccollector.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MvmGC.h"

using namespace mvm;

GCAllocator   *Collector::allocator = 0;
#ifdef HAVE_PTHREAD
GCThread      *Collector::threads;
#endif


int           Collector::status;

GCChunkNode    *Collector::used_nodes;
GCChunkNode    *Collector::unused_nodes;

unsigned int   Collector::current_mark;

int  Collector::_collect_freq_auto;
int  Collector::_collect_freq_maybe;
int  Collector::_since_last_collection;

bool Collector::_enable_auto;
bool Collector::_enable_maybe;
bool Collector::_enable_collection;

void Collector::do_collect() {
  GCChunkNode  *cur;
#ifdef SERVICE
  mvm::Thread::get()->MyVM->_since_last_collection = _collect_freq_auto;
#else
  _since_last_collection = _collect_freq_auto;
#endif

  current_mark++;

  unused_nodes->attrape(used_nodes);

  mvm::Thread* th = mvm::Thread::get();
  th->MyVM->startCollection();

  threads->synchronize();

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

  status = stat_finalize;

  /* finalize */
  GCChunkNode  finalizable;
  finalizable.attrape(unused_nodes);

  
  /* kill everyone */
  GCChunkNode *next = 0;
  for(cur=finalizable.next(); cur!=&finalizable; cur=next) {
    next = cur->next();
    allocator->reject_chunk(cur);
  }

  status = stat_alloc;

  th->MyVM->endCollection();
  threads->collectionFinished();
  th->MyVM->wakeUpFinalizers();
  th->MyVM->wakeUpEnqueue();
}

void Collector::collect_unprotect() {
  if(_enable_collection && (status == stat_alloc)) {
    status = stat_collect;
    do_collect();
  }
}

void Collector::gcStats(size_t *_no, size_t *_nbb) {
   register unsigned int n, tot;
   register GCChunkNode *cur;
   lock();
   for(n=0, tot=0, cur=used_nodes->next(); cur!=used_nodes; cur=cur->next(), n++)
     tot += cur->nbb();
   unlock();
   *_no = n;
   *_nbb = tot;
}

