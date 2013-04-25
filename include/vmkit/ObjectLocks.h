//===----------- ObjectLocks.h - Object based locks -----------------------===//
//
//                      The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef VMKIT_OBJECT_LOCKS_H
#define VMKIT_OBJECT_LOCKS_H

#include "vmkit/Allocator.h"
#include "vmkit/GC.h"
#include "vmkit/Cond.h"
#include "vmkit/Locks.h"
#include "vmkit/Thread.h"

namespace vmkit {

class FatLock;
class LockSystem;

class LockingThread {
public:
  /// varcond - Condition variable when the thread needs to be awaken from
  /// a wait.
  ///
  vmkit::Cond varcond;

  /// interruptFlag - Has this thread been interrupted?
  ///
  uint32 interruptFlag;

  /// nextWaiting - Next thread waiting on the same monitor.
  ///
  LockingThread* nextWaiting;
  
  /// prevWaiting - Previous thread waiting on the same monitor.
  ///
  LockingThread* prevWaiting;

  /// waitsOn - The lock on which the thread is waiting on.
  ///
  FatLock* waitsOn;

  static const unsigned int StateRunning = 0;
  static const unsigned int StateWaiting = 1;
  static const unsigned int StateInterrupted = 2;

  /// state - The current state of this thread: Running, Waiting or Interrupted.
  uint32 state;

  LockingThread() {
    interruptFlag = 0;
    nextWaiting = NULL;
    prevWaiting = NULL;
    waitsOn = NULL;
    state = StateRunning;
  }

  bool wait(gc* object, LockSystem& table, struct timeval* info, bool timed);
  void notify(gc* object, LockSystem& table);
  void notifyAll(gc* object, LockSystem& table);
};


class FatLock : public vmkit::PermanentObject {
private:
  vmkit::LockRecursive internalLock;
  vmkit::SpinLock spinLock;
  uint32_t waitingThreads;
  uint32_t lockingThreads;
  LockingThread* firstThread;
  gc* associatedObject;
  uint32_t index;
  FatLock* nextFreeLock;
public:
  FatLock(uint32_t index, gc* object);
  word_t getID();
  int tryAcquire() { return internalLock.tryLock(); }
  bool acquire(gc* object);
  void acquireAll(gc* object, word_t count);
  void release(gc* object, LockSystem& table);
  vmkit::Thread* getOwner();
  bool owner();
  void setAssociatedObject(gc* obj);
  gc* getAssociatedObject() { return associatedObject; }
  gc** getAssociatedObjectPtr() { return &associatedObject; }

  friend class LockSystem;
  friend class LockingThread;
  friend class ThinLock;
};


/// LockSystem - This class manages all Java locks used by the applications.
/// Each JVM must own an instance of this class and allocate Java locks
/// with it.
///
class LockSystem {
  friend class FatLock;
public:
  
  // Fixed values. With these values, an index is on 18 bits.
  static const uint32_t GlobalSize = 128;
  static const uint32_t BitIndex = 11;
  static const uint32_t IndexSize = 1 << BitIndex;
  static const uint32_t BitMask = IndexSize - 1;
  static const uint32_t MaxLocks = GlobalSize * IndexSize;

  vmkit::BumpPtrAllocator& allocator;

  /// LockTable - The global table that will hold the locks. The table is
  /// a two-dimensional array, and only one entry is created, so that
  /// the lock system does not eat up all memory on startup.
  ///  
  FatLock* ** LockTable;
  
  /// currentIndex - The current index in the tables. Always incremented,
  /// never decremented.
  ///
  uint32_t currentIndex;
 
  /// freeLock - The list of locks that are allocated and available.
  ///
  FatLock* freeLock;
 
  /// threadLock - Spin lock to protect the currentIndex field.
  ///
  vmkit::SpinLock threadLock;
  
  /// allocate - Allocate a FatLock.
  ///
  FatLock* allocate(gc* obj); 
 
  /// deallocate - Put a lock in the free list lock.
  ///
  void deallocate(FatLock* lock);

  /// LockSystem - Default constructor. Initialize the table.
  ///
  LockSystem(vmkit::BumpPtrAllocator& allocator);

  /// getLock - Get a lock from an index in the table.
  ///
  FatLock* getLock(uint32_t index) {
    return LockTable[index >> BitIndex][index & BitMask];
  }

  FatLock* getFatLockFromID(word_t ID);
};

class ThinLock {
public:
  
  // The header of an object that has a thin lock implementation is like the
  // following:
  //
  //    x      xxx xxxx xxxx        xxxx xxxx xxxx        xxxx xxxx
  //    ^      ^^^ ^^^^ ^^^^        ^^^^ ^^^^ ^^^^        ^^^^ ^^^^
  //    1           11                    12                  8
  // fat lock    thread id       thin lock count + hash     GC bits

  static const uint64_t FatMask = 1LL << (kThreadStart > 0xFFFFFFFFLL ? 61LL : 31LL);

  static const uint64_t NonLockBits = HashBits + GCBits;
  static const uint64_t NonLockBitsMask = ((1LL << NonLockBits) - 1LL);

  static const uint64_t ThinCountMask = 0xFFFFFFFFLL & ~(FatMask | kThreadIDMask | NonLockBitsMask);
  static const uint64_t ThinCountShift = NonLockBits;
  static const uint64_t ThinCountAdd = 1LL << NonLockBits;

  /// initialise - Initialise the value of the lock.
  ///
  static void removeFatLock(FatLock* fatLock, LockSystem& table);

  /// overflowThinlock - Change the lock of this object to a fat lock because
  /// we have reached the maximum number of locks.
  static void overflowThinLock(gc* object, LockSystem& table);
 
  /// changeToFatlock - Change the lock of this object to a fat lock. The lock
  /// may be in a thin lock or fat lock state.
  static FatLock* changeToFatlock(gc* object, LockSystem& table);

  /// acquire - Acquire the lock.
  static void acquire(gc* object, LockSystem& table);

  /// release - Release the lock.
  static void release(gc* object, LockSystem& table);

  /// owner - Returns true if the curren thread is the owner of this object's
  /// lock.
  static bool owner(gc* object, LockSystem& table);

  /// getFatLock - Get the fat lock is the lock is a fat lock, 0 otherwise.
  static FatLock* getFatLock(gc* object, LockSystem& table);
};

} // end namespace vmkit

#endif // VMKIT_OBJECT_LOCKS_H
