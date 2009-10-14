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

#include "mvm/GC/GC.h"

#include "types.h"

namespace n3 {

class VMCommonClass;
class VMField;
class VMObject;
class VMThread;

struct N3VirtualTable : VirtualTable {
	uintptr_t  print;
	uintptr_t  hashCode;

	void *operator new(size_t size, mvm::BumpPtrAllocator &allocator, size_t totalVtSize);

  N3VirtualTable();
  N3VirtualTable(N3VirtualTable *vmobjVt, uint32 baseVtSize, uint32 totalVtSize=-1);
  N3VirtualTable(uintptr_t d, uintptr_t o, uintptr_t t, uintptr_t p, uintptr_t h);

	static uint32 baseVtSize();
};

class LockObj : public mvm::Object {
public:
	static N3VirtualTable  _VT;
  mvm::LockRecursive     lock;
  std::vector<VMThread*> threads;

  static LockObj* allocate();

	static void _destroy(LockObj *);
	static void _print(const LockObj *, mvm::PrintBuffer *);
  
  void notify();
  void notifyAll();
  void wait(VMThread* th);
  void remove(VMThread* th);

  void aquire();
  void release();
  bool owner();
};

#define VALUE_OFFSET 3

class VMObject : public mvm::Object {
public:
  VMCommonClass* classOf;
  LockObj* lockObj;

  static mvm::Lock* globalLock;
  static const llvm::Type* llvmType;

	static void _print(const VMObject *, mvm::PrintBuffer *);
	static void _trace(VMObject *);
  
  void aquire();
  void unlock();
  void waitIntern(struct timeval *info, bool timed);
  void wait();
  void timedWait(struct timeval &info);
  void notify();
  void notifyAll();
  void initialise(VMCommonClass* cl);

  static llvm::Constant* classOffset();
  
  bool instanceOf(VMCommonClass* cl);


	N3VirtualTable *getN3VirtualTable() { return (N3VirtualTable*)getVirtualTable(); }

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) VMThread::get()->getVM()->nullPointerException("");
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
