//===-------------- gcthread.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include "gccollector.h"

using namespace mvm;

void GCThread::waitStacks() {
	_stackLock.lock();
	while(_nb_collected < _nb_threads)
		_stackCond.wait(&_stackLock);
	_stackLock.unlock();
}

void GCThread::synchronize() {
	int signo = GCCollector::siggc();
  mvm::Thread* self = mvm::Thread::get();
  assert(self && "No thread local data for this thread");
	current_collector = self;
	_nb_collected = 0;

 	for(mvm::Thread* cur = (mvm::Thread*)self->next(); cur != self; 
      cur = (mvm::Thread*)cur->next()) {
    cur->kill(signo);
	}

	GCCollector::siggc_handler(signo);
	
  waitStacks();
}

int GCLockRecovery::verify_recall(gc_lock_recovery_fct_t fct, int a0, int a1,
                                  int a2, int a3, int a4, int a5, int a6,
                                  int a7) {
	if(selfOwner()) {
    _fct = fct;    _args[0] = a0; _args[1] = a1; _args[2] = a2; _args[3] = a3;
		_args[4] = a4; _args[5] = a5; _args[6] = a6; _args[7] = a7;
		return 0;
	} else
		return 1;
}
