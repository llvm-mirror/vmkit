//===--- ctlock.cc - Common threads implementation of locks ---------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>

#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"
#include "cterror.h"
#include <sys/time.h>


using namespace mvm;

Lock::Lock(bool recursive) {
  pthread_mutexattr_t attr;

  // Initialize the mutex attributes
  int errorcode = pthread_mutexattr_init(&attr);
  assert(errorcode == 0); 

  // Initialize the mutex as a recursive mutex, if requested, or normal
  // otherwise.
  int kind = ( recursive  ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL );
  errorcode = pthread_mutexattr_settype(&attr, kind);
  assert(errorcode == 0); 

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__) && \
    !defined(__DragonFly__)
  // Make it a process local mutex
  errorcode = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
#endif

  // Initialize the mutex
  errorcode = pthread_mutex_init(&internalLock, &attr);
  assert(errorcode == 0); 

  // Destroy the attributes
  errorcode = pthread_mutexattr_destroy(&attr);
  assert(errorcode == 0);

  owner = 0;
}

Lock::~Lock() {
  pthread_mutex_destroy((pthread_mutex_t*)&internalLock);
}

bool Lock::selfOwner() {
  return owner == mvm::Thread::get();
}

mvm::Thread* Lock::getOwner() {
  return owner;
}

void LockNormal::lock() {
  pthread_mutex_lock((pthread_mutex_t*)&internalLock);
  owner = mvm::Thread::get();
}

void LockNormal::unlock() {
  owner = 0;
  pthread_mutex_unlock((pthread_mutex_t*)&internalLock);
}

void LockRecursive::lock() {
  pthread_mutex_lock((pthread_mutex_t*)&internalLock);
  if (!owner) owner = mvm::Thread::get();
  ++n;
}

void LockRecursive::unlock() {
  --n;
  if (n == 0) owner = 0;
  pthread_mutex_unlock((pthread_mutex_t*)&internalLock);
}

int LockRecursive::unlockAll() {
  int res = n;
  while (n) unlock();
  return res;
}

void LockRecursive::lockAll(int count) {
  for (int i = 0; i < count; ++i) lock();
}

Cond::Cond() {
  int errorcode = pthread_cond_init((pthread_cond_t*)&internalCond, NULL);
  assert(errorcode == 0); 
}

Cond::~Cond() {
  pthread_cond_destroy((pthread_cond_t*)&internalCond);
}

void Cond::broadcast() {
  pthread_cond_broadcast((pthread_cond_t*)&internalCond);
}

void Cond::wait(Lock* l) {
  pthread_cond_wait((pthread_cond_t*)&internalCond,
                    (pthread_mutex_t*)&(l->internalLock));
}

void Cond::signal() {
  pthread_cond_signal((pthread_cond_t*)&internalCond);
}

int Cond::timedWait(Lock* l, struct timeval *ref) {
  
  struct timespec timeout; 
  struct timeval now; 
  struct timezone tz; 
  gettimeofday(&now, &tz); 
  timeout.tv_sec = now.tv_sec + ref->tv_sec; 
  timeout.tv_nsec = now.tv_usec + ref->tv_usec;
  return pthread_cond_timedwait((pthread_cond_t*)&internalCond, 
                                (pthread_mutex_t*)&(l->internalLock), &timeout);
}
