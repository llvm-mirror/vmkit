//===--------- VMCache.h - Inline cache for virtual calls -----------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_VM_CACHE_H
#define N3_VM_CACHE_H

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "llvm/DerivedTypes.h"

#include "types.h"

namespace n3 {

class Assembly;
class Enveloppe;
class UTF8;
class VMClass;

class CacheNode : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  void* methPtr;
  VMClass* lastCible;
  CacheNode* next;
  Enveloppe* enveloppe;
  bool box;
  
  static const llvm::Type* llvmType;

  static CacheNode* allocate();

};

class Enveloppe : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const;
  
  CacheNode *firstCache;
  mvm::Lock* cacheLock;
  VMMethod* originalMethod;

  static const llvm::Type* llvmType;

  static Enveloppe* allocate(VMMethod* orig);

};

} // end namespace n3

#endif
