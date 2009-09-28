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
#include "mvm/Threads/Thread.h"
#include "MvmGC.h"
#include "gcerror.h"

using namespace mvm;


extern "C" void MarkAndTrace(gc* gc) {
  Collector::markAndTrace((void*)gc);
}

extern "C" void* gcmalloc(size_t sz, VirtualTable* VT) {
  mvm::Thread::get()->startNative(1);
  void* res = Collector::gcmalloc(VT, sz);
  mvm::Thread::get()->endNative();
  return res;
}


extern "C" void conditionalSafePoint() {
  mvm::Thread::get()->startNative(1);
  Collector::traceStackThread();  
  mvm::Thread::get()->endNative();
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

void Collector::scanObject(void* obj) {
  if (obj) {
    GCChunkNode *node = o2node(obj);

#if 0//def WITH_LLVM_GCC
    assert(node && "No node in precise GC mode");
#endif
  
    if (node && !Collector::isMarked(node)) {
      node->remove();
      node->append(Collector::used_nodes);
      Collector::mark(node);
    }
  }
}

void Collector::siggc_handler(int) {
  mvm::Thread* th = mvm::Thread::get();
  th->inGC = true;

  
  Collector::threads->stackLock();

  if (Collector::threads->cooperative && !th->doYield) {
    // I was previously blocked, and I'm running late: someone else collected
    // my stack, and the GC has finished already. Just unlock and return.
    Collector::threads->stackUnlock();
    th->inGC = false;
    return;
  }
 
  // I woke up while a GC was happening, and no-one has collected my stack yet.
  // Do it now.
  if (!th->stackScanned) {
    th->MyVM->getScanner()->scanStack(th);
    Collector::threads->another_mark();
    th->stackScanned = true;
  }

  // Wait for the collection to finish.
  Collector::threads->waitCollection();
  Collector::threads->stackUnlock();
  
  // If the current thread is not the collector thread, this means that the
  // collection is finished. Set inGC to false.
  if(th != threads->getCurrentCollector())
    th->inGC = false;
}

void Collector::traceForeignThreadStack(mvm::Thread* th) {
  Collector::threads->stackLock();
 
  // The thread may have waken up during this GC. In this case, it may also
  // have collected its stack. Don't scan it then.
  if (!th->stackScanned) {
    th->MyVM->getScanner()->scanStack(th);
    Collector::threads->another_mark();
    th->stackScanned = true;
  }

  Collector::threads->stackUnlock();
}
