//===------ JavaCache.cpp - Inline cache for virtual calls -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "types.h"

using namespace jnjvm;

Enveloppe::~Enveloppe() {
  CacheNode* cache = firstCache;
  CacheNode* next = firstCache;
  mvm::BumpPtrAllocator& allocator = ctpInfo->classDef->classLoader->allocator;
  while(next) {
    next = cache->next;
    cache->~CacheNode();
    allocator.Deallocate(cache);
    cache = next;
  }
}

CacheNode::CacheNode(Enveloppe* E) {
  lastCible = 0;
  methPtr = 0;
  next = 0;
  enveloppe = E;
#ifdef ISOLATE_SHARING
  definingCtp = 0;
#endif
}

void Enveloppe::initialise(UserConstantPool* ctp, uint32 i) {
  mvm::BumpPtrAllocator& allocator = ctp->classDef->classLoader->allocator;
  firstCache = new(allocator) CacheNode(this);
  ctpInfo = ctp;
  index = i;
}
