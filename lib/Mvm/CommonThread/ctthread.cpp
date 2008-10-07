//===-------------- ctthread.cc - Mvm common threads ----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Key.h"
#include "mvm/Threads/Thread.h"

#include <pthread.h>
#include <signal.h>
#include <time.h>

using namespace mvm;

void Thread::yield() {
  sched_yield();
}

int Thread::self() {
  return (int)pthread_self();
}

void Thread::yield(unsigned int *c) {
  if(++(*c) & 3)
    sched_yield();
  else {
    struct timespec ts; 
    ts.tv_sec = 0;
    ts.tv_nsec = 2000;
    nanosleep(&ts, 0); 
  }
}

int Thread::kill(int tid, int signo) {
  return pthread_kill((pthread_t)tid, signo);
}

void Thread::exit(int value) {
  pthread_exit((void*)value);
}

void* ThreadKey::get()        { 
  pthread_key_t k = (pthread_key_t)val;
  return (void *)pthread_getspecific(k);
}

void  ThreadKey::set(void *v) {
  pthread_key_t k = (pthread_key_t)val;
  pthread_setspecific(k, v);
}

ThreadKey::ThreadKey(void (*_destr)(void *)) {
  pthread_key_create((pthread_key_t*)&val, _destr);
}

ThreadKey::ThreadKey() {
  pthread_key_create((pthread_key_t*)&val, NULL);
}

void ThreadKey::initialise() {
  pthread_key_create((pthread_key_t*)&val, NULL);
}

void Thread::initialise() {
  Thread::threadKey = new mvm::Key<Thread>();
  Thread* th = new Thread();
  mvm::Thread::set(th);
}

int Thread::start(int *tid, int (*fct)(void *), void *arg) {
  int res = pthread_create((pthread_t *)tid, 0, (void * (*)(void *))fct, arg);
  pthread_detach(*(pthread_t *)tid);
  return res;
}
