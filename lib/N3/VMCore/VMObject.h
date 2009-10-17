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

#include "N3MetaType.h"

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
  mvm::LockRecursive     *lock;
  std::vector<VMThread*> *threads;

  static LockObj* allocate();
	static void _destroy(LockObj *);
	static void _print(const LockObj *, mvm::PrintBuffer *);
  
  static void notify(LockObj*);
  static void notifyAll(LockObj*);
  static void wait(LockObj*, VMThread* th);
  static void remove(LockObj*, VMThread* th);

  static void aquire(LockObj*);
  static void release(LockObj*);
  static bool owner(LockObj*);
};

#define VALUE_OFFSET 3

class VMObject : public mvm::Object {
public:
  VMCommonClass* classOf;
  LockObj*       lockObj;

  static mvm::Lock*        globalLock;
  static const llvm::Type* llvmType;

	static void _print(const VMObject *, mvm::PrintBuffer *);
	static void _trace(VMObject *);
  
  static void aquire(VMObject *self);
  static void unlock(VMObject *self);
  static void waitIntern(VMObject *self, struct timeval *info, bool timed);
  static void wait(VMObject *self);
  static void timedWait(VMObject *self, struct timeval &info);
  static void notify(VMObject *self);
  static void notifyAll(VMObject *self);
  static void initialise(VMObject *self, VMCommonClass* cl);

  static llvm::Constant* classOffset();
  
  static bool instanceOf(VMObject *self, VMCommonClass* cl);


	static N3VirtualTable *getN3VirtualTable(VMObject *self) { llvm_gcroot(self, 0); return *((N3VirtualTable**)self); }

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) VMThread::get()->getVM()->nullPointerException("");
#endif

};


} // end namespace n3

#endif
