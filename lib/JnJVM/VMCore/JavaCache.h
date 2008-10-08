//===------- JavaCache.h - Inline cache for virtual calls -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Caches in JnJVM are used when invoking interface methods in the JVM
// bytecode. A cache is a linked-list of {class, method pointer} pairs that
// were already encountered during the execution of the program. Its efficiency
// is based on the hypothesis that for one invokeinterface location, the "this"
// arguments will share most of the times the same class.
//
// At a given time, the first entry in the linked list is the last class of the
// "this" parameter.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CACHE_H
#define JNJVM_JAVA_CACHE_H

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#include "JnjvmConfig.h"

namespace jnjvm {

class Enveloppe;
class UserClass;
class UserConstantPool;

/// CacheNode - A {class, method pointer} pair.
class CacheNode {
public:

  /// methPtr - The method pointer of this cache.
  void* methPtr;

  /// lastCible - The class of this cache.
  UserClass* lastCible;

  /// next - The next cache.
  CacheNode* next;
  
  /// enveloppe - The container to which this class belongs to.
  Enveloppe* enveloppe;

#ifdef ISOLATE_SHARING
  ///definingClass - The class that defined the method being called.
  ///
  UserConstantPool* definingCtp;
#endif

  /// CacheNode - Creates a CacheNode with empty values.
  CacheNode(Enveloppe* E);
};

/// Enveloppe - A reference to the linked list of CacheNode.
class Enveloppe {
public:
  
  /// ~Enveloppe - Deletes all CacheNode in the linked list.
  ~Enveloppe();

  /// firstCache - The first entry in the linked list, hence the last
  /// class occurence for a given invokeinterface call.
  CacheNode *firstCache;

  /// ctpInfo - The constant pool info that owns the invokeinterface
  /// bytecode. This is used to resolve the interface call at its first
  /// occurence.
  UserConstantPool* ctpInfo;

  /// cacheLock - The linked list may be modified by concurrent thread. This
  /// lock ensures that the list stays consistent.
  mvm::Lock* cacheLock;

  /// index - The index in the constant pool of the interface method.
  uint32 index;

  /// Enveloppe - Allocates the linked list with the given constant pool info
  /// at the given index, so as the resolution process knows which interface
  /// method the invokeinterface bytecode references.
  Enveloppe(UserConstantPool* info, uint32 index);

};

} // end namespace jnjvm

#endif
