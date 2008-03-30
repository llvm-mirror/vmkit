//===-------------- gcthread.cc - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gccollector.h"

using namespace mvm;

void GCThread::waitStacks() {
	_stackLock.lock();
	while(_nb_collected < _nb_threads)
		_stackCond.wait(&_stackLock);
	_stackLock.unlock();
}

void GCThread::synchronize() {
 	GCThreadCollector *cur;
	int signo = GCCollector::siggc();
	int reached = 0;
	collector_tid = Thread::self();
	
	_nb_collected = 0;
 	for(cur=(GCThreadCollector *)base.next(); cur!=&base; cur=(GCThreadCollector *)cur->next()) {
		int t = cur->tid();
		if(t != Thread::self())
			Thread::kill(cur->tid(), signo);
		else
			reached = 1;
	}
	if(reached)  /* moi aussi je dois collecter... */
		GCCollector::siggc_handler(signo);
	waitStacks();
}

int GCLockRecovery::verify_recall(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
	if(owner() == Thread::self()) {
		_fct = fct;    _args[0] = a0; _args[1] = a1; _args[2] = a2; _args[3] = a3;
		_args[4] = a4; _args[5] = a5; _args[6] = a6; _args[7] = a7;
		return 0;
	} else
		return 1;
}
