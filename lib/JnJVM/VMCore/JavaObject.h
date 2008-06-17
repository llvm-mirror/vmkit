//===----------- JavaObject.h - Java object definition -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_OBJECT_H
#define JNJVM_JAVA_OBJECT_H

#include <vector>

#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#include "JavaClass.h"

namespace jnjvm {

class JavaField;
class JavaObject;
class JavaThread;
class UTF8;

/// JavaCond - This class maintains a list of threads blocked on a wait. 
/// notify and notifyAll will change the state of one or more of these threads.
///
class JavaCond {
private:
  
  /// threads - The list of threads currently waiting.
  ///
  std::vector<JavaThread*> threads;

public:
  
  /// notify - The Java notify: takes the first thread in the list and wakes
  /// it up.
  void notify();
  
  /// notifyAll - The Java notifyAll: wakes all threads in the list.
  ///
  void notifyAll();

  /// wait - The Java wait: the thread blocks waiting to be notified.
  ///
  void wait(JavaThread* th);

  /// remove - Remove the thread from the list. This is called by threads being
  /// interrupted or timed out on a wait.
  void remove(JavaThread* th);
};


/// LockObj - This class represents a Java monitor.
///
class LockObj : public mvm::Object {

  friend class JavaObject;

private:


  /// lock - The internal lock of this object lock.
  ///
  mvm::Lock *lock;

  /// varcond - The condition variable for wait, notify and notifyAll calls.
  ///
  JavaCond* varcond;

  /// allocate - Allocates a lock object. Only the internal lock is allocated.
  ///
  static LockObj* allocate();
  
  /// myLock - Returns the lock object of this object, allocating it if
  /// non-existant. This uses the big lock to make the object lock unique.
  ///
  static LockObj* myLock(JavaObject* obj);

  /// acquire - Acquires the lock.
  ///
  void acquire() {
    lock->lock();
  }

  /// release - Releases the lock.
  ///
  void release() {
    lock->unlock();
  }
  
  /// owner - Returns if the current thread owns this lock.
  ///
  bool owner() {
    return mvm::Lock::selfOwner(lock);
  }
  
  /// getCond - Returns the conditation variable of this lock, allocating it
  /// if non-existant.
  ///
  JavaCond* getCond() {
    if (!varcond) varcond = new JavaCond();
    return varcond;
  }

public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  ~LockObj();
  LockObj();
};


/// JavaObject - This class represents a Java object.
///
class JavaObject : public mvm::Object {
private:
  
  /// waitIntern - internal wait on a monitor
  ///
  void waitIntern(struct timeval *info, bool timed);

  
public:
  static VirtualTable* VT;

  /// classOf - The class of this object.
  ///
  CommonClass* classOf;

  /// lock - The monitor of this object. Most of the time null.
  ///
  uint32 lock;

  /// wait - Java wait. Makes the current thread waiting on a monitor.
  ///
  void wait();

  /// timedWait - Java timed wait. Makes the current thread waiting on a
  /// monitor for the given amount of time.
  ///
  void timedWait(struct timeval &info);
  
  /// notify - Java notify. Notifies a thread from the availability of the
  /// monitor.
  ///
  void notify();
  
  /// notifyAll - Java notifyAll. Notifies all threads from the availability of
  /// the monitor.
  ///
  void notifyAll();
  
  /// initialise - Initialises the object.
  ///
  void initialise(CommonClass* cl) {
    this->classOf = cl; 
    this->lock = 0;
  }

  /// instanceOfString - Is this object's class of type the given name?
  ///
  bool instanceOfString(const UTF8* name) {
    if (!this) return false;
    else return this->classOf->isOfTypeName(name);
  }

  /// instanceOf - Is this object's class of type the given class?
  ///
  bool instanceOf(CommonClass* cl) {
    if (!this) return false;
    else return this->classOf->isAssignableFrom(cl);
  }

  /// acquire - Acquire the lock on this object.
  void acquire();

  /// release - Release the lock on this object
  void release();

  /// changeToFatlock - Change the lock of this object to a fat lock. The lock
  /// may be in thin lock or in fat lock.
  LockObj* changeToFatlock();

  /// overflowThinlock -Change the lock of this object to a fat lock because
  /// we have reached 0xFF locks.
  void overflowThinlock();
  
  /// owner - Returns true if the curren thread is the owner of this object's
  /// lock.
  bool owner();

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) JavaThread::get()->isolate->nullPointerException("");
#endif
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  LockObj* lockObj() {
    if (lock & 0x80000000) {
      return (LockObj*)(lock << 1);
    } else {
      return 0;
    }
  }
};


} // end namespace jnjvm

#endif
