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
	int signo = Collector::siggc();
  mvm::Thread* self = mvm::Thread::get();
  assert(self && "No thread local data for this thread");
	current_collector = self;
	_nb_collected = 0;

 	for(mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
      cur = (mvm::Thread*)cur->next()) {
    cur->kill(signo);
	}

	Collector::siggc_handler(signo);
	
  waitStacks();
}
