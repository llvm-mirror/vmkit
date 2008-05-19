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

class JavaCond : public mvm::Object {
public:
  static VirtualTable* VT;
  std::vector<JavaThread*> threads;
  
  static JavaCond* allocate();

  void notify();
  void notifyAll();
  void wait(JavaThread* th);
  void remove(JavaThread* th);

  virtual void TRACER;
};


class LockObj : public mvm::Object {
public:
  static VirtualTable* VT;
  mvm::Lock *lock;
  JavaCond* varcond;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  static LockObj* allocate();
  static LockObj* myLock(JavaObject* obj);

  void aquire();
  void release();
  bool owner();
};

class JavaObject : public mvm::Object {
public:
  static VirtualTable* VT;
  CommonClass* classOf;
  LockObj* lockObj;

  static mvm::Lock* globalLock;
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  
  void waitIntern(struct timeval *info, bool timed);
  void wait();
  void timedWait(struct timeval &info);
  void notify();
  void notifyAll();
  void initialise(CommonClass* cl) {
    this->classOf = cl; 
    this->lockObj = 0;
  }

  bool instanceOfString(const UTF8* name) {
    if (!this) return false;
    else return this->classOf->isOfTypeName(name);
  }

  bool instanceOf(CommonClass* cl) {
    if (!this) return false;
    else return this->classOf->isAssignableFrom(cl);
  }

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) JavaThread::get()->isolate->nullPointerException("");
#endif
  
};


} // end namespace jnjvm

#endif
