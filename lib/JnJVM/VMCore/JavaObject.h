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
#include "mvm/Threads/Thread.h"

#include "types.h"

#include "JavaClass.h"

namespace jnjvm {

class JavaObject;
class JavaThread;

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
  mvm::LockRecursive lock;

  /// varcond - The condition variable for wait, notify and notifyAll calls.
  ///
  JavaCond varcond;

public:
  /// allocate - Allocates a lock object. Only the internal lock is allocated.
  ///
  static LockObj* allocate(JavaObject*);
  
  /// acquire - Acquires the lock.
  ///
  void acquire() {
    lock.lock();
  }
  
  /// acquireAll - Acquires the lock nb times.
  void acquireAll(uint32 nb) {
    lock.lockAll(nb);
  }

  /// release - Releases the lock.
  ///
  void release() {
    lock.unlock();
  }
  
  /// owner - Returns if the current thread owns this lock.
  ///
  bool owner() {
    return lock.selfOwner();
  }
  
  /// getCond - Returns the conditation variable of this lock, allocating it
  /// if non-existant.
  ///
  JavaCond* getCond() {
    return &varcond;
  }

  static VirtualTable* VT;

  ~LockObj() {}
  LockObj() {}
};


/// JavaObject - This class represents a Java object.
///
class JavaObject : public mvm::Object {
private:
  
  /// waitIntern - internal wait on a monitor
  ///
  void waitIntern(struct timeval *info, bool timed);
  
  /// classOf - The class of this object.
  ///
  UserCommonClass* classOf;

public:

  /// getClass - Returns the class of this object.
  ///
  UserCommonClass* getClass() const {
    return classOf;
  }


  /// lock - The monitor of this object. Most of the time null.
  ///
  mvm::ThinLock<LockObj, JavaObject> lock;

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
 
  /// overflowThinLokc - Notify that the thin lock has overflowed.
  ///
  void overflowThinLock() {
    lock.overflowThinLock();
  }

  /// initialise - Initialises the object.
  ///
  void initialise(UserCommonClass* cl) {
    this->classOf = cl; 
  }

  /// instanceOf - Is this object's class of type the given class?
  ///
  bool instanceOf(UserCommonClass* cl) {
    if (!this) return false;
    else return this->classOf->isAssignableFrom(cl);
  }

  /// acquire - Acquire the lock on this object.
  void acquire() {
    lock.acquire();
  }

  /// release - Release the lock on this object
  void release() {
    lock.release();
  }

  /// owner - Returns true if the current thread is the owner of this object's
  /// lock.
  bool owner() {
    return lock.owner();
  }

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) JavaThread::get()->getJVM()->nullPointerException("");
#endif
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;


  /// lockObj - Get the LockObj if the lock is a fat lock.
  LockObj* lockObj() {
    return lock.getFatLock();
  }

  /// decapsulePrimitive - Based on the signature argument, decapsule
  /// obj as a primitive and put it in the buffer.
  ///
  void decapsulePrimitive(Jnjvm* vm, uintptr_t &buf, const Typedef* signature);

};


} // end namespace jnjvm

#endif
