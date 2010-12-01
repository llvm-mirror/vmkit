//===--------------- gcinit.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC.h"

using namespace mvm;

static const size_t def_collect_freq_auto = 64*1024*1024;
static const size_t def_collect_freq_maybe = 64*1024*1024;


void Collector::initialise() {
 
  used_nodes = new GCChunkNode();
  unused_nodes = new GCChunkNode();
  allocator = new GCAllocator();
   
  used_nodes->alone();
  unused_nodes->alone();

  current_mark = 0;
  status = stat_alloc;

  _collect_freq_auto = def_collect_freq_auto;
  _collect_freq_maybe = def_collect_freq_maybe;
  
  _since_last_collection = _collect_freq_auto;

  _enable_auto = 1;
  _enable_collection = 1;
  _enable_maybe = 1;

}

void Collector::destroy() {
  delete allocator;
  allocator = 0;
}

