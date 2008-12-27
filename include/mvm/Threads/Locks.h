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

#include "mvm/Threads/Thread.h"

namespace mvm {

extern "C" uint8  llvm_atomic_cmp_swap_i8(uint8* ptr,  uint8 cmp,
                                          uint8 val);
extern "C" uint16 llvm_atomic_cmp_swap_i16(uint16* ptr, uint16 cmp,
                                           uint16 val);
extern "C" uint32 llvm_atomic_cmp_swap_i32(uint32* ptr, uint32 cmp,
                                           uint32 val);
extern "C" uint64 llvm_atomic_cmp_swap_i64(uint64* ptr, uint64 cmp,
                                           uint64 val);

// TODO: find what macro for gcc < 4.2

#define __sync_bool_compare_and_swap_32(ptr, cmp, val) \
  mvm::llvm_atomic_cmp_swap_i32((uint32*)(ptr), (uint32)(cmp), \
                           (uint32)(val)) == (uint32)(cmp)

#if (__WORDSIZE == 64)

#define __sync_bool_compare_and_swap(ptr, cmp, val) \
  mvm::llvm_atomic_cmp_swap_i64((uint64*)(ptr), (uint64)(cmp), \
                           (uint64)(val)) == (uint64)(cmp)

#define __sync_val_compare_and_swap(ptr, cmp,val) \
  mvm::llvm_atomic_cmp_swap_i64((uint64*)(ptr), (uint64)(cmp), \
                           (uint64)(val))


#else



#define __sync_bool_compare_and_swap(ptr, cmp, val) \
  mvm::llvm_atomic_cmp_swap_i32((uint32*)(ptr), (uint32)(cmp), \
                           (uint32)(val)) == (uint32)(cmp)

#define __sync_val_compare_and_swap(ptr, cmp,val) \
  mvm::llvm_atomic_cmp_swap_i32((uint32*)(ptr), (uint32)(cmp), \
                           (uint32)(val))
#endif


class Cond;
class LockNormal;
class LockRecursive;
class Thread;

/// Lock - This class is an abstract class for declaring recursive and normal
/// locks.
///
class Lock {
  friend class Cond;
protected:
  /// owner - Which thread is currently holding the lock?
  ///
  mvm::Thread* owner;

  /// internalLock - The lock implementation of the platform.
  ///
  pthread_mutex_t internalLock;

public:

  /// Lock - Creates a lock, recursive if rec is true.
  ///
  Lock(bool rec);
  
  /// ~Lock - Give it a home.
  ///
  virtual ~Lock();
  
  /// lock - Acquire the lock.
  ///
  virtual void lock() = 0;

  /// unlock - Release the lock.
  ///
  virtual void unlock() = 0;

  /// selfOwner - Is the current thread holding the lock?
  ///
  bool selfOwner();

  /// getOwner - Get the thread that is holding the lock.
  ///
  mvm::Thread* getOwner();
  
};

/// LockNormal - A non-recursive lock.
class LockNormal : public Lock {
public:
  LockNormal() : Lock(false) {}

  virtual void lock();
  virtual void unlock();

};

/// LockRecursive - A recursive lock.
class LockRecursive : public Lock {
private:
  
  /// n - Number of times the lock has been locked.
  ///
  int n;

public:
  LockRecursive() : Lock(true) { n = 0; }
  
  virtual void lock();
  virtual void unlock();
  
  /// recursionCount - Get the number of times the lock has been locked.
  ///
  int recursionCount() { return n; }

  /// unlockAll - Unlock the lock, releasing it the number of times it is held.
  /// Return the number of times the lock has been locked.
  ///
  int unlockAll();

  /// lockAll - Acquire the lock count times.
  ///
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
            (uintptr_t)__sync_val_compare_and_swap((uintptr_t)&lock,
                                                   currentLock, val);
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


/// SpinLock - This class implements a spin lock. A spin lock is OK to use
/// when it is held during short period of times. It is CPU expensive
/// otherwise.
class SpinLock {
public:

  /// locked - Is the spin lock locked?
  ///
  uint8 locked;
  
  /// SpinLock - Initialize the lock as not being held.
  ///
  SpinLock() { locked = 0; }


  /// acquire - Acquire the spin lock, doing an active loop. When the lock
  /// is already held, yield the processor.
  ///
  void acquire() {
    uint32 count = 0;
    while (!(llvm_atomic_cmp_swap_i8(&locked, 0, 1)))
      mvm::Thread::yield(&count);
  }

  /// release - Release the spin lock. This must be called by the thread
  /// holding it.
  ///
  void release() { locked = 0; }
};


} // end namespace mvm

#endif // MVM_LOCKS_H
