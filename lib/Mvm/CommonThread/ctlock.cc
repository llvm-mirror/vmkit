//===--- ctlock.cc - Common threads implementation of locks ---------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"
#include "cterror.h"
#include <sys/time.h>


using namespace mvm;

inline int atomic_test_and_pass(unsigned int *ptr) {

#if defined(__ppc__) || defined(__powerpc__)
  int result;
# if defined(__MACH__)
  const int val=1;

  asm volatile("1:\n"
#if 0
      "  lwz    %0,0(%1)\n"  /* result <- ptr[0] */
      "  cmpwi  %0,0\n"
      "  bne    1b\n"
#endif
      "  lwarx  %0,0,%1\n"   /* result <- ptr[0] */
      "  cmpwi  %0,0\n"
      "  bne    1b\n"
      "  stwcx. %2,0,%1\n"  /* ptr[0] <- 1 (si pas d'autre accès) */
      "  bne-   1b\n"
      "  isync\n"           /* empeche toute execution d'instruction après le lock!!! */
      :"=&r"(result): "r"(ptr), "r"(val): "cr0", "memory");
  /* le & empeche gcc d'utiliser le même register pour result et les entrées... */
# else
  const int val=1;

  asm volatile("1:\n"
      "  lwarx  %0,0,%1\n"   /* result <- ptr[0] */
      "  cmpwi  %0,0\n"
      "  bne    1b\n"
      "  stwcx. %2,0,%1\n"  /* ptr[0] <- 1 (si pas d'autre accès) */
      "  bne-   1b\n"
      "  isync\n"           /* empeche toute execution d'instruction après le lock!!! */
      :"=&r"(result): "r"(ptr), "r"(val): "cr0", "memory");
  /* le & empeche gcc d'utiliser le même register pour result et les entrées... */
# endif

  return result;
#elif defined(__i386__)
	// asm ("bts $1, %1; sbbl %0, %0":"=r" (result):"m" (*ptr):"memory");
	unsigned int old;
	int c=0;
	do {
		asm volatile("movl   $1,%0;"
								 "xchgl  %0, %1" : "=&r"(old) : "m"(*ptr));
		if(!old)
			return 0;
		if(!(++c & 0xf))
			Thread::yield();
	} while(1);
#else
#error "I do not know  how to do an atomic test and pass on your machine"
#endif
}

Lock *Lock::allocRecursive() {
  return new LockRecursive();
}

Lock *Lock::allocNormal() {
  return new LockNormal();
}

void Lock::destroy(Lock *l) {
}

bool Lock::selfOwner(Lock *l) {
  return l->owner() == Thread::self();
}

int Lock::getOwner(Lock *l) {
  return l->owner();
}

void SpinLock::slock() {
  atomic_test_and_pass(&value);
}

void LockNormal::my_lock(Lock *l) {
  unsigned int c = 0;

  l->slock();

  while(l->owner()) {
    l->sunlock();
    Thread::yield(&c);
    l->slock();
  }
  l->owner(Thread::self());
  l->sunlock();
}

void LockNormal::my_unlock(Lock *l) { 
  l->owner(0); 
}

int LockNormal::my_trylock(Lock *) {
  ctfatal("not implemented");
  return -1;
}

void LockRecursive::my_lock(Lock *l) {
  unsigned int c = 0;

  int self = Thread::self();

  l->slock();
  while(l->owner() && (l->owner() != self)) {
    l->sunlock();
    Thread::yield(&c);
    l->slock();
  }
  l->owner(self);
  ((LockRecursive *)l)->n++;
  l->sunlock();
}

  void LockRecursive::my_unlock(Lock *l) { 
    if(!--((LockRecursive *)l)->n)
      l->owner(0); 
  }

int LockRecursive::my_trylock(Lock *) {
  ctfatal("not implemented");
  return -1;
}

int LockRecursive::recursion_count(Lock *l){
  return ((LockRecursive*)l)->n;
}

int LockRecursive::my_unlock_all(Lock *l){
  int count = ((LockRecursive*)l)->n;
  ((LockRecursive*)l)->n = 0;
  l->owner(0);
  return count;
}

void LockRecursive::my_lock_all(Lock *l, int count){
  LockRecursive::my_lock(l);
  ((LockRecursive*)l)->n = count;
}




void Cond::wait(Lock *l) {
  unsigned int n=0;
  unsigned int my_barrier = no_barrier;

  n_wait++;			/* un attente de plus */
  while(1) {
    if(no_barrier == my_barrier)			/* pas de broadcast */
      if(go) {												/* un signal? */
        n_wait--;											/* pus une attente en moins */
        go--;													/* pour le pochain */
        return;
      } else {												/* à l'ouest rien de nouveau */
        l->unlock();									/* je rend gentillement le lock */
        Thread::yield(&n); 				/* j'attends qu'on me libère... */
        l->lock();										/* je reprends mon lock */
      }
      else															/* on a eu un broadcast */
        return;
  }
}

int Cond::timed_wait(Lock *l, struct timeval *ref) {
  unsigned int		n=0;
  unsigned int		my_barrier = no_barrier;
  struct timeval	max, cur;

  gettimeofday(&max, 0);
  timeradd(&max, ref, &max);

  n_wait++;
  while(1) {
    if(no_barrier == my_barrier)			/* pas de broadcast */
      if(go) {												/* un signal? */
        n_wait--;											/* puis une attente en moins */
        go--;													/* pour le pochain */
        return 0;
      } else {												/* à l'ouest rien de nouveau */
        gettimeofday(&cur, 0);
        if(timercmp(&cur, &max, >=)) {/* timesout écoulé */
          n_wait--;										/* on n'attend plus */
          return 1;										/* c'est nif */
        }

        l->unlock();
        timersub(&max, &cur, &cur);		/* le reste */
        if(!cur.tv_sec && (cur.tv_usec <= 2))
          Thread::yield();				/* j'attends qu'on me libère... */
        else
          Thread::yield(&n);			/* j'attends qu'on me libère... */
        l->lock();										/* je reprends mon lock */
      }
      else															/* on a eu un broadcast */
        return 0;
  }
}

void Cond::broadcast() {
  no_barrier++;
  n_wait = 0;
}

void Cond::signal() {
  if(n_wait>go) go++;
}


extern "C" void lock_C(Lock* l) {
  return l->lock();
}

extern "C" void unlock_C(Lock* l) {
  return l->unlock();
}

Cond* Cond::allocCond() {
  return new Cond();
}
