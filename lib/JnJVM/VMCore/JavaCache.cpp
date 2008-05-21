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

void CacheNode::initialise() {
  this->lastCible = 0;
  this->methPtr = 0;
  this->next = 0;
}

Enveloppe* Enveloppe::allocate(JavaCtpInfo* ctp, uint32 index) {
  Enveloppe* enveloppe = new Enveloppe();
  enveloppe->firstCache = new CacheNode();
  enveloppe->firstCache->initialise();
  enveloppe->firstCache->enveloppe = enveloppe;
  enveloppe->cacheLock = mvm::Lock::allocNormal();
  enveloppe->ctpInfo = ctp;
  enveloppe->index = index;
  return enveloppe;
}

