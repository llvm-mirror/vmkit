//===------------------ Locks.h - Thread locks ----------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_LOCKS_H
#define MVM_LOCKS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mvm/GC/GC.h"

namespace mvm {

class SpinLock {
 	unsigned int value;
public:
  static VirtualTable *VT; 
 	SpinLock() { value = 0; }
	
 	void slock();
 	void sunlock() {
 		value = 0;
 	}
};

class Lock : public SpinLock {
protected:
	void		(*xlock)(Lock *);
	void		(*xunlock)(Lock *);
	int			(*xtrylock)(Lock *);

	int			_owner;

public:
	inline Lock() { _owner = 0; }
	inline int	owner() { return _owner; }
	inline void owner(int o) { _owner = o; }

	void lock() { 
		xlock(this); 
	}

	void unlock() { 
		xunlock(this);
	}

	int trylock() { 
		int res = xtrylock(this); 
		return res;
	}

	static Lock *allocNormal();
	static Lock *allocRecursive();
	static void destroy(Lock *);

	static bool selfOwner(Lock *);
	static int getOwner(Lock *);
  
};

class LockNormal : public Lock {
	static void		my_lock(Lock *);
	static void		my_unlock(Lock *);
	static int		my_trylock(Lock *);
public:
	LockNormal() {
    xlock = my_lock; xunlock = my_unlock; xtrylock = my_trylock;
  }

  void initialise() {
    xlock = my_lock; xunlock = my_unlock; xtrylock = my_trylock;
  }
};
	
class LockRecursive : public Lock {
	int n;

	static void		my_lock(Lock *);
	static void		my_unlock(Lock *);
	static int		my_trylock(Lock *);
public:
	LockRecursive() {
    xlock = my_lock; xunlock = my_unlock; xtrylock = my_trylock; n = 0;
  }
  
  void initialise() {
    xlock = my_lock; xunlock = my_unlock; xtrylock = my_trylock; n = 0;
  }
  
  static int recursion_count(Lock *);
  static int my_unlock_all(Lock *);
  static void my_lock_all(Lock *, int count);
};

} // end namespace mvm

#endif // MVM_LOCKS_H
