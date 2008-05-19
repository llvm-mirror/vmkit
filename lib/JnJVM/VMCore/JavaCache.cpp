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

void Enveloppe::destroyer(size_t sz) {
  delete cacheLock;
}

void CacheNode::print(mvm::PrintBuffer* buf) const {
  buf->write("CacheNode<");
  buf->write(" in ");
  enveloppe->print(buf);
  buf->write(">");
}

void Enveloppe::print(mvm::PrintBuffer* buf) const {
  buf->write("Enveloppe<>");
}

void CacheNode::initialise() {
  this->lastCible = 0;
  this->methPtr = 0;
  this->next = 0;
}

Enveloppe* Enveloppe::allocate(JavaCtpInfo* ctp, uint32 index) {
  Enveloppe* enveloppe = vm_new(ctp->classDef->isolate, Enveloppe)();
  enveloppe->firstCache = vm_new(ctp->classDef->isolate, CacheNode)();
  enveloppe->firstCache->initialise();
  enveloppe->firstCache->enveloppe = enveloppe;
  enveloppe->cacheLock = mvm::Lock::allocNormal();
  enveloppe->ctpInfo = ctp;
  enveloppe->index = index;
  return enveloppe;
}

