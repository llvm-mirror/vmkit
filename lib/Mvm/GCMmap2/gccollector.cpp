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
SpinLock      Collector::_globalLock;

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

int Collector::verbose = 0;

void Collector::do_collect() {
  GCChunkNode  *cur;
  _since_last_collection = _collect_freq_auto;

  current_mark++;

  unused_nodes->attrape(used_nodes);

  mvm::Thread* th = mvm::Thread::get();
  th->MyVM->rendezvous.startRV();
  th->MyVM->startCollection();

  th->MyVM->rendezvous.synchronize();

  mvm::Thread* tcur = th;

  // (1) Trace the VM.
  th->MyVM->tracer(0);

  // (2) Trace the threads.
  do {
    tcur->scanStack(0);
    tcur->tracer(0);
    tcur = (mvm::Thread*)tcur->next();
  } while (tcur != th);

  // (3) Trace stack objects.
  for(cur = used_nodes->next(); cur != used_nodes; cur = cur->next())
    trace(cur);

  // Go back to the previous node.
  cur = cur->prev();

  // (4) Trace the weak reference queue.
  th->MyVM->scanWeakReferencesQueue(0);

  // (5) Trace the soft reference queue.
  th->MyVM->scanSoftReferencesQueue(0);
  
  // (6) Trace the finalization queue.
  th->MyVM->scanFinalizationQueue(0);

  // (7) Trace the phantom reference queue.
  th->MyVM->scanPhantomReferencesQueue(0);

  // (8) Trace the new objects added by queues.
  for(cur = cur->next(); cur != used_nodes; cur = cur->next())
    trace(cur);


  // Finalize.
  GCChunkNode finalizable;
  finalizable.attrape(unused_nodes);

  // We have stopped collecting, go back to alloc state.
  status = stat_alloc;
  
  // Wake up all threads.
  th->MyVM->rendezvous.finishRV();
  th->MyVM->endCollection();
  
  // Kill unreachable objects.
  GCChunkNode *next = 0;
  for(cur=finalizable.next(); cur!=&finalizable; cur=next) {
    next = cur->next();
    allocator->reject_chunk(cur);
  }
  
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

