//===---------------- VMObject.h - VM object definition -------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_VM_OBJECT_H
#define N3_VM_OBJECT_H

#include <vector>

#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/ExecutionEngine/GenericValue.h"

#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

namespace n3 {

class VMCommonClass;
class VMField;
class VMObject;
class VMThread;
class UTF8;

class VMCond : public mvm::Object {
public:
  static VirtualTable* VT;
  std::vector<VMThread*> threads;
  
  static VMCond* allocate();

  void notify();
  void notifyAll();
  void wait(VMThread* th);
  void remove(VMThread* th);

  virtual void TRACER;
};


class LockObj : public mvm::Object {
public:
  static VirtualTable* VT;
  mvm::Lock *lock;
  VMCond* varcond;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  static LockObj* allocate();
  void aquire();
  void release();
  bool owner();
};

#define VALUE_OFFSET 3

class VMObject : public mvm::Object {
public:
  static VirtualTable* VT;
  VMCommonClass* classOf;
  LockObj* lockObj;

  static mvm::Lock* globalLock;
  static const llvm::Type* llvmType;
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  
  static VMObject* allocate(VMCommonClass* cl);
  void aquire();
  void unlock();
  void waitIntern(struct timeval *info, bool timed);
  void wait();
  void timedWait(struct timeval &info);
  void notify();
  void notifyAll();
  void initialise(VMCommonClass* cl);

  static llvm::ConstantInt* classOffset();
  
  bool instanceOf(VMCommonClass* cl);


#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) VMThread::get()->vm->nullPointerException("");
#endif
  
  llvm::GenericValue operator()(VMField* field);
  void operator()(VMField* field, float val);
  void operator()(VMField* field, double val);
  void operator()(VMField* field, sint64 val);
  void operator()(VMField* field, sint32 val);
  void operator()(VMField* field, VMObject* val);
  void operator()(VMField* field, bool val);

};


} // end namespace n3

#endif
