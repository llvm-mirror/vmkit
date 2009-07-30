//===-------------- gcthread.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include "MvmGC.h"

using namespace mvm;

void GCThread::waitStacks() {
	_stackLock.lock();
	while(_nb_collected < _nb_threads)
		_stackCond.wait(&_stackLock);
	_stackLock.unlock();
}

void GCThread::synchronize() {
	
  if (cooperative) {
    mvm::Thread* self = mvm::Thread::get();
    assert(self && "No thread local data for this thread");
	  current_collector = self;
	  _nb_collected = 0;
 	 
    // Lock stacks. Changes on the doYield flag of threads must be
    // protected, as threads may wake up after being blocked in native code
    // and join the collection.
    stackLock();

    mvm::Thread* cur = self;
    do {
      cur->stackScanned = false;
      cur->doYield = true;
      cur = (mvm::Thread*)cur->next();
    } while (cur != self);
    
    // Unlock now. Each running thread will scan its stack.
    stackUnlock();

    // Scan the stacks of currently blocked threads.
 	  for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      void* val = cur->getLastSP();
      // If val is null, this means that the thread woke up, and is
      // joining the collection. We are sure the thread will scan its stack.
      if (val) Collector::traceForeignThreadStack(cur, val);
	  }

    // Finally, scan my stack too!
	  Collector::siggc_handler(0);

    // And wait for other threads to finish.
    waitStacks();
  } else {
    int signo = Collector::siggc();
    mvm::Thread* self = mvm::Thread::get();
    self->stackScanned = false;
    assert(self && "No thread local data for this thread");
	  current_collector = self;
	  _nb_collected = 0;

 	  for (mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
         cur = (mvm::Thread*)cur->next()) {
      cur->stackScanned = false;
      cur->kill(signo);
	  }

	  Collector::siggc_handler(signo);
	
    waitStacks();
  }
}
