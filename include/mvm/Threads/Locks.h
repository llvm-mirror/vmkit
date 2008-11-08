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

#include <pthread.h>

namespace mvm {

class Cond;
class LockNormal;
class LockRecursive;
class Thread;

class Lock {
  friend class Cond;
protected:
  mvm::Thread* owner;
  pthread_mutex_t internalLock;

public:
  Lock(bool rec);
  
  ~Lock();
  
  virtual void lock() = 0;
  virtual void unlock() = 0;

  bool selfOwner();
  mvm::Thread* getOwner();
  
};

class LockNormal : public Lock {
public:
  LockNormal() : Lock(false) {}
  virtual void lock();
  virtual void unlock();

};
  
class LockRecursive : public Lock {
private:
  int n;

public:
  LockRecursive() : Lock(true) {
    n = 0;
  }
  
  virtual void lock();
  virtual void unlock();
  
  int recursionCount() { 
    return n;
  }
  int unlockAll();
  void lockAll(int count);
};


} // end namespace mvm

#endif // MVM_LOCKS_H
