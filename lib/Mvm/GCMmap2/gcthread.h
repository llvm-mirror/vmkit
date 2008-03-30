//===--------------- gcthread.h - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_THREAD_H_
#define _GC_THREAD_H_

#include "ctcircular.h"
#include "mvm/GC/GC.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Key.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h" // only for self

// class GCAllocator;

namespace mvm {

class GCThreadCollector : public CircularBase {
 	void							*_base_sp;
 	int								_cur_mark;
 	int								_tid;
	
public:
	inline GCThreadCollector() {}
 	inline GCThreadCollector(GCThreadCollector *pred, int t, void *p, int m) : CircularBase(pred) {
		_base_sp = p;
  	_cur_mark = m;
		_tid = t;
	}

	/* cette fonction n'est appelée que dans deux cas: soit parsqu'un thread quit */
	/* dans ce cas, tout est déjà fait */
	/* soit parce que le collecteur quitte et dans ce cas, toute la mémoire est bien libérée */
	/* This function is only called in two cases:
	 *   1) When a thread quits, in which case everything is already done.
	*    2) When the collector quits, in which case all memory is freed.
	*/
 	inline 	~GCThreadCollector() {}
	
 	inline int					tid()						{ return _tid; }
 	inline unsigned int current_mark()	{ return _cur_mark; }
 	inline void					current_mark(unsigned int m)	{ _cur_mark = m; }
 	inline unsigned int **base_sp()	{ return (unsigned int **)_base_sp; }
};

class GCLockRecovery : public LockNormal {
	gc_lock_recovery_fct_t _fct;
	int                    _args[8];
	
public:
	inline GCLockRecovery() { _fct = 0; }

	int verify_recall(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7);

	inline void unlock_dont_recovery() { 
		if(owner() == Thread::self()) {
			LockNormal::unlock(); 
		}
	}

	inline void unlock() {
		if(_fct) {
			gc_lock_recovery_fct_t tmp = _fct;
			int l[8];
			l[0] = _args[0]; l[1] = _args[1]; l[2] = _args[2]; l[3] = _args[3]; 
			l[4] = _args[4]; l[5] = _args[5]; l[6] = _args[6]; l[7] = _args[7]; 
			_fct = 0;
			LockNormal::unlock();
			tmp(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
		} else
			LockNormal::unlock();
	}
};

class GCThread {
 	Key<GCThreadCollector>	_loc;
 	GCThreadCollector				base;
	GCLockRecovery         _globalLock;    /* lock global pour gcmalloc */
	LockNormal              _stackLock;     /* lock de la pile pour la syncro */
	Cond                    _stackCond;     /* condition pour déverouiller les autres taches (write protect) */
	Cond                    _collectionCond;/* condition pour débloquer le collecteur */
 	unsigned int						_nb_threads;    /* nombre de threads actifs */
 	unsigned int						_nb_collected;  /* nombre de threads ayant collecté */
	int                     collector_tid;  /* il ne faut pas le synchroniser celui là */

public:

	inline void lock()   { _globalLock.lock(); }
	inline void unlock() { _globalLock.unlock(); }
	inline void unlock_dont_recovery() { _globalLock.unlock_dont_recovery(); }
	inline int isStable(gc_lock_recovery_fct_t fct, int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) { 
		return _globalLock.verify_recall(fct, a0, a1, a2, a3, a4, a5, a6, a7);
	}

	inline void stackLock() { _stackLock.lock(); }
	inline void stackUnlock() { _stackLock.unlock(); }

	void        waitStacks();
	void        waitCollection();
	inline void collectionFinished() { _collectionCond.broadcast(); }
	inline void collectorGo() { _stackCond.broadcast(); }

	inline void cancel() { 
		_nb_collected = _nb_threads;  /* toutes les piles sont collectées */
		collectorGo();                /* débloque les autres threads sur la phase de collection de pile */
		collectionFinished();         /* débloque les mutateurs */
	}

 	inline GCThreadCollector *myloc() { return _loc.get(); }

 	inline void another_mark() { _nb_collected++; }

	void synchronize();

	inline void remove(GCThreadCollector *loc) {
		lock();
		loc->remove();
		_nb_threads--;
		delete loc;
		unlock();
	}

	inline void inject(void *sp, int m) {
		GCThreadCollector *me = _loc.get();
		if(me)
			gcfatal("thread %d is already in pool for allocator %p", Thread::self(), this);
		lock(); /* the new should be protected */
		me = new GCThreadCollector(&base, Thread::self(), sp, m);
		_nb_threads++;
		_loc.set(me);
		unlock();
	}
};

}
#endif
