//===------- JavaCache.h - Inline cache for virtual calls -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CACHE_H
#define JNJVM_JAVA_CACHE_H

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

namespace jnjvm {

class Class;
class Enveloppe;
class JavaCtpInfo;

class CacheNode {
public:

  void* methPtr;
  Class* lastCible;
  CacheNode* next;
  Enveloppe* enveloppe;

  void initialise();

};

class Enveloppe {
public:
  
  ~Enveloppe();
  CacheNode *firstCache;
  JavaCtpInfo* ctpInfo;
  mvm::Lock* cacheLock;
  uint32 index;

  static Enveloppe* allocate(JavaCtpInfo* info, uint32 index);

};

} // end namespace jnjvm

#endif
