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
#include <cassert>
#include <cstdio>

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

#ifndef WITH_LLVM_GCC

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
  
private:
  virtual void unsafeLock(int n) = 0;
  virtual int unsafeUnlock() = 0;

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
  Lock();
  
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
  friend class Cond;
private:
  virtual void unsafeLock(int n) {
    owner = mvm::Thread::get();
  }
  
  virtual int unsafeUnlock() {
    owner = 0;
    return 0;
  }
public:
  LockNormal() : Lock() {}

  virtual void lock();
  virtual void unlock();

};

/// LockRecursive - A recursive lock.
class LockRecursive : public Lock {
  friend class Cond;
private:
  
  /// n - Number of times the lock has been locked.
  ///
  int n;

  virtual void unsafeLock(int a) {
    n = a;
    owner = mvm::Thread::get();
  }
  
  virtual int unsafeUnlock() {
    int ret = n;
    n = 0;
    owner = 0;
    return ret;
  }

public:
  LockRecursive() : Lock() { n = 0; }
  
  virtual void lock();
  virtual void unlock();
  virtual int tryLock();

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

  static const uint64_t ThinCountMask = 0xFF000;
  static const uint64_t ThinCountShift = 12;
  static const uint64_t ThinCountAdd = 0x1000;
  static const uint64_t GCMask = 0xFFF;



#ifdef WITH_LLVM_GCC
extern "C" void __llvm_gcroot(const void**, void*) __attribute__((nothrow));
#define llvm_gcroot(a, b) __llvm_gcroot((const void**)&a, b)
#else
#define llvm_gcroot(a, b)
#endif

  class FatLockNoGC {
  public:
    static void gcroot(void* val, void* unused) 
      __attribute__ ((always_inline)) {}

    static uintptr_t mask() {
      return 0;
    }
  };
  
  class FatLockWithGC {
  public:
    static void gcroot(void* val, void* unused) 
      __attribute__ ((always_inline)) {
      llvm_gcroot(val, unused);
    }
    
    static uintptr_t mask() {
      return GCMask;
    }
  };


/// ThinLock - This class is an implementation of thin locks. The template class
/// TFatLock is a virtual machine specific fat lock.
///
template <class TFatLock, class Owner, class IsGC>
class ThinLock {
public:
  uintptr_t lock;




  /// overflowThinlock - Change the lock of this object to a fat lock because
  /// we have reached 0xFF locks.
  void overflowThinLock(Owner* O) {
    IsGC::gcroot(O, 0);
    TFatLock* obj = TFatLock::allocate(O);
    obj->acquireAll((ThinCountMask >> ThinCountShift) + 1);
    uintptr_t oldLock = lock;
    lock = obj->getID() | (oldLock & IsGC::mask());
  }
 
  /// initialise - Initialise the value of the lock.
  ///
  void initialise() {
    lock = lock & IsGC::mask();
  }
  
  /// ThinLock - Calls initialize.
  ThinLock() {
    initialise();
  }

  
  /// changeToFatlock - Change the lock of this object to a fat lock. The lock
  /// may be in a thin lock or fat lock state.
  TFatLock* changeToFatlock(Owner* O) {
    IsGC::gcroot(O, 0);
    if (!(lock & FatMask)) {
      TFatLock* obj = TFatLock::allocate(O);
      uint32 count = (lock & ThinCountMask) >> ThinCountShift;
      obj->acquireAll(count + 1);
      uintptr_t oldLock = lock;
      lock = obj->getID() | (oldLock & IsGC::mask());
      return obj;
    } else {
      TFatLock* res = TFatLock::getFromID(lock);
      assert(res && "Lock deallocated while held.");
      return res;
    }
  }

  /// acquire - Acquire the lock.
  void acquire(Owner* O) {
    IsGC::gcroot(O, 0);
start:
    uint64_t id = mvm::Thread::get()->getThreadID();
    uintptr_t oldValue = lock;
    uintptr_t newValue = id | (lock & IsGC::mask());
    uintptr_t val = __sync_val_compare_and_swap(&lock, oldValue & IsGC::mask(),
                                                newValue);

    if (val != (oldValue & IsGC::mask())) {
      //fat!
      if (!(val & FatMask)) {
        if ((val & Thread::IDMask) == id) {
          if ((val & ThinCountMask) != ThinCountMask) {
            lock += ThinCountAdd;
          } else {
            overflowThinLock(O);
          }
        } else {
          TFatLock* obj = TFatLock::allocate(O);
          uintptr_t val = obj->getID();
loop:
          while (lock & ~IsGC::mask()) {
            if (lock & FatMask) {
              obj->deallocate();
              goto end;
            }
            else mvm::Thread::yield();
          }
        
          oldValue = lock;
          newValue = val | (lock & IsGC::mask());
          uintptr_t test = __sync_val_compare_and_swap(&lock,
                                                       oldValue & IsGC::mask(),
                                                       newValue);
          if (test != (oldValue & IsGC::mask())) goto loop;
          if (!obj->acquire(O)) goto start;
        }
      } else {

end:
        TFatLock* obj = TFatLock::getFromID(lock);
        if (obj) {
          if (!obj->acquire(O)) goto start;
        } else {
          goto start;
        }
      }
    }

    assert(owner() && "Not owner after quitting acquire!");
  }

  /// release - Release the lock.
  void release(Owner* O) {
    IsGC::gcroot(O, 0);
    assert(owner() && "Not owner when entering release!");
    uint64 id = mvm::Thread::get()->getThreadID();
    if ((lock & ~IsGC::mask()) == id) {
      lock = lock & IsGC::mask();
    } else if (lock & FatMask) {
      TFatLock* obj = TFatLock::getFromID(lock);
      assert(obj && "Lock deallocated while held.");
      obj->release(O);
    } else {
      lock -= ThinCountAdd;
    }
  }

  /// broadcast - Wakes up all threads waiting for this lock.
  ///
  void broadcast() {
    if (lock & FatMask) {
      TFatLock* obj = TFatLock::getFromID(lock);
      assert(obj && "Lock deallocated while held.");
      obj->broadcast();
    }
  }
  
  /// signal - Wakes up one thread waiting for this lock.
  void signal() {
    if (lock & FatMask) {
      TFatLock* obj = TFatLock::getFromID(lock);
      assert(obj && "Lock deallocated while held.");
      obj->signal();
    }
  }
  
  /// owner - Returns true if the curren thread is the owner of this object's
  /// lock.
  bool owner() {
    if (lock & FatMask) {
      TFatLock* obj = TFatLock::getFromID(lock);
      if (obj) return obj->owner();
    } else {
      uint64 id = mvm::Thread::get()->getThreadID();
      if ((lock & Thread::IDMask) == id) return true;
    }
    return false;
  }

  mvm::Thread* getOwner() {
    if (lock & FatMask) {
      TFatLock* obj = TFatLock::getFromID(lock);
      if (obj) return obj->getOwner();
      return 0;
    } else {
      return (mvm::Thread*)(lock & mvm::Thread::IDMask);
    }
  }

  /// getFatLock - Get the fat lock is the lock is a fat lock, 0 otherwise.
  TFatLock* getFatLock() {
    if (lock & FatMask) {
      return TFatLock::getFromID(lock);
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
  uint32 locked;
  
  /// SpinLock - Initialize the lock as not being held.
  ///
  SpinLock() { locked = 0; }


  /// acquire - Acquire the spin lock, doing an active loop.
  ///
  void acquire() {
    for (uint32 count = 0; count < 1000; ++count) {
      uint32 res = __sync_val_compare_and_swap(&locked, 0, 1);
      if (!res) return;
    }
    
    while (__sync_val_compare_and_swap(&locked, 0, 1))
      mvm::Thread::yield();
  }

  void lock() { acquire(); }

  /// release - Release the spin lock. This must be called by the thread
  /// holding it.
  ///
  void release() { locked = 0; }
  
  void unlock() { release(); }
};


} // end namespace mvm

#endif // MVM_LOCKS_H
