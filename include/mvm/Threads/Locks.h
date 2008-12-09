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

#include "mvm/JIT.h"
#include "mvm/Threads/Thread.h"

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
  
  virtual ~Lock();
  
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

#if (__WORDSIZE == 64)
  static const uint64_t FatMask = 0x8000000000000000;
#else
  static const uint64_t FatMask = 0x80000000;
#endif

  static const uint64_t ThinMask = 0x7FFFFF00;
  static const uint64_t ReservedMask = 0X7FFFFFFF;
  static const uint64_t ThinCountMask = 0xFF;


/// ThinLock - This class is an implementation of thin locks with reservation.
/// The creator of the lock reserves this lock so that a lock only needs
/// a comparison and not an expensive compare and swap. The template class
/// TFatLock is a virtual machine specific fat lock.
///
template <class TFatLock, class Owner>
class ThinLock {
  uintptr_t lock;

public:
  /// overflowThinlock - Change the lock of this object to a fat lock because
  /// we have reached 0xFF locks.
  void overflowThinLock(Owner* O = 0) {
    TFatLock* obj = TFatLock::allocate(O);
    obj->acquireAll(256);
    lock = ((uintptr_t)obj >> 1) | FatMask;
  }
 
  /// initialise - Initialise the value of the lock to the thread ID that is
  /// creating this lock.
  ///
  void initialise() {
    lock = (uintptr_t)mvm::Thread::get()->getThreadID();
  }
  
  /// ThinLock - Calls initialize.
  ThinLock() {
    initialise();
  }

  
  /// changeToFatlock - Change the lock of this object to a fat lock. The lock
  /// may be in a thin lock or fat lock state.
  TFatLock* changeToFatlock(Owner* O) {
    if (!(lock & FatMask)) {
      TFatLock* obj = TFatLock::allocate(O);
      uintptr_t val = ((uintptr_t)obj >> 1) | FatMask;
      uint32 count = lock & ThinCountMask;
      obj->acquireAll(count);
      lock = val;
      return obj;
    } else {
      return (TFatLock*)(lock << 1);
    }
  }
 

  /// acquire - Acquire the lock.
  void acquire(Owner* O = 0) {
    uint64_t id = mvm::Thread::get()->getThreadID();
    if ((lock & ReservedMask) == id) {
      lock |= 1;
    } else if ((lock & ThinMask) == id) {
      if ((lock & ThinCountMask) == ThinCountMask) {
        overflowThinLock(O);
      } else {
        ++lock;
      }
    } else {
      uintptr_t currentLock = lock & ThinMask;
      uintptr_t val = 
        (uintptr_t)__sync_val_compare_and_swap((uintptr_t)&lock, currentLock, 
                                             (id + 1));
      if (val != currentLock) {
        if (val & FatMask) {
end:
          //fat lock!
          TFatLock* obj = (TFatLock*)(lock << 1);
          obj->acquire();
        } else {
          TFatLock* obj = TFatLock::allocate(O);
          val = ((uintptr_t)obj >> 1) | FatMask;
          uint32 count = 0;
loop:
          if (lock & FatMask) goto end;

          while ((lock & ThinCountMask) != 0) {
            if (lock & FatMask) {
#ifdef USE_GC_BOEHM
              delete obj;
#endif
              goto end;
            }
            else {
              mvm::Thread::yield(&count);
            }
          }
        
          currentLock = lock & ThinMask;
          uintptr_t test = 
            (uintptr_t)__sync_val_compare_and_swap((uintptr_t)&lock, currentLock,
                                                   val);
          if (test != currentLock) goto loop;
          obj->acquire();
        }
      }
    }
  }

  /// release - Release the lock.
  void release() {
    uint64 id = mvm::Thread::get()->getThreadID();
    if ((lock & ThinMask) == id) {
      --lock;
    } else {
      TFatLock* obj = (TFatLock*)(lock << 1);
      obj->release();
    } 
  }

  /// broadcast - Wakes up all threads waiting for this lock.
  ///
  void broadcast() {
    if (lock & FatMask) {
      TFatLock* obj = (TFatLock*)(lock << 1);
      obj->broadcast();
    }
  }
  
  /// signal - Wakes up one thread waiting for this lock.
  void signal() {
    if (lock & FatMask) {
      TFatLock* obj = (TFatLock*)(lock << 1);
      obj->signal();
    }
  }
  
  /// owner - Returns true if the curren thread is the owner of this object's
  /// lock.
  bool owner() {
    uint64 id = mvm::Thread::get()->getThreadID();
    if ((lock & ThinMask) == id) return true;
    if (lock & FatMask) {
      TFatLock* obj = (TFatLock*)(lock << 1);
      return obj->owner();
    }
    return false;
  }

  /// getFatLock - Get the fat lock is the lock is a fat lock, 0 otherwise.
  TFatLock* getFatLock() {
    if (lock & FatMask) {
      return (TFatLock*)(lock << 1);
    } else {
      return 0;
    }
  }
};

} // end namespace mvm

#endif // MVM_LOCKS_H
