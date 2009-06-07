//===---------------- gc.cc - Mvm Garbage Collector -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <csetjmp>
#include <cstdlib>

#include "mvm/GC/GC.h"
#include "MvmGC.h"
#include "gcerror.h"

using namespace mvm;


extern "C" void MarkAndTrace(gc* gc) {
  Collector::markAndTrace((void*)gc);
}

extern "C" void* gcmalloc(size_t sz, VirtualTable* VT) {
  return Collector::gcmalloc(VT, sz);
}

void GCThread::waitCollection() {
  mvm::Thread* th = mvm::Thread::get();
  unsigned int cm = Collector::current_mark;

  if(th != current_collector) {
    collectorGo();
    while((Collector::current_mark == cm) && 
          (Collector::status == Collector::stat_collect))
      _collectionCond.wait(&_stackLock);
  }
}

void Collector::siggc_handler(int) {
  mvm::Thread* th = mvm::Thread::get();

  jmp_buf buf;
  setjmp(buf);
  
  Collector::threads->stackLock();
  
  if(!th) /* The thread is being destroyed */
    Collector::threads->another_mark();
  else {
    register unsigned int  **cur = (unsigned int**)(void*)&buf;
    register unsigned int  **max = (unsigned int**)th->baseSP;
    
    GCChunkNode *node;
    
    for(; cur<max; cur++) {
      if((node = o2node(*cur)) && (!Collector::isMarked(node))) {
        node->remove();
        node->append(Collector::used_nodes);
        Collector::mark(node);
      }
    }
    
    Collector::threads->another_mark();
    Collector::threads->waitCollection();
  }
  Collector::threads->stackUnlock();
}
