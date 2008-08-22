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
  delete cacheLock;
  CacheNode* cache = firstCache;
  CacheNode* next = firstCache;
  while(next) {
    next = cache->next;
    delete cache;
    cache = next;
  }
}

CacheNode::CacheNode(Enveloppe* E) {
  lastCible = 0;
  methPtr = 0;
  next = 0;
  enveloppe = E;
}

Enveloppe::Enveloppe(JavaConstantPool* ctp, uint32 i) {
  firstCache = new CacheNode(this);
  cacheLock = mvm::Lock::allocNormal();
  ctpInfo = ctp;
  index = i;
}
