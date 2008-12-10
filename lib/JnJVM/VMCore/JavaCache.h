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

#include "mvm/Allocator.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#include "JnjvmConfig.h"

namespace jnjvm {

class Enveloppe;
class Signdef;
class UserClass;
class UTF8;

/// CacheNode - A {class, method pointer} pair.
class CacheNode : public mvm::PermanentObject {
public:

  /// methPtr - The method pointer of this cache.
  void* methPtr;

  /// lastCible - The class of this cache.
  UserClass* lastCible;

  /// next - The next cache.
  CacheNode* next;
  
  /// enveloppe - The container to which this class belongs to.
  Enveloppe* enveloppe;

  /// CacheNode - Creates a CacheNode with empty values.
  CacheNode(Enveloppe* E) {
    lastCible = 0;
    methPtr = 0;
    next = 0;
    enveloppe = E;
  }
};

/// Enveloppe - A reference to the linked list of CacheNode.
class Enveloppe : public mvm::PermanentObject {
public:
  
  /// firstCache - The first entry in the linked list, hence the last
  /// class occurence for a given invokeinterface call.
  ///
  CacheNode *firstCache;
  
  /// methodName - The name of the method to be called.
  ///
  const UTF8* methodName;

  /// methodSign - The signature of the method to be called.
  ///
  Signdef* methodSign;

  /// cacheLock - The linked list may be modified by concurrent thread. This
  /// lock ensures that the list stays consistent.
  ///
  mvm::LockNormal cacheLock;

  /// allocator - Reference to the allocator in order to know where to allocate
  /// cache nodes.
  ///
  mvm::BumpPtrAllocator* allocator;
  
  /// bootCache - The first cache allocated for the enveloppe.
  ///
  CacheNode bootCache;

  /// Evenloppe - Default constructor. Does nothing.
  ///
  Enveloppe() : bootCache(this) {}

  /// Enveloppe - Allocates the linked list with the given name and signature
  /// so as the resolution process knows which interface method the
  /// invokeinterface bytecode refers to.
  ///
  Enveloppe(mvm::BumpPtrAllocator& Alloc, const UTF8* name, Signdef* sign) : 
    bootCache(this) {
    
    initialise(Alloc, name, sign);
  }

  /// initialise - Initialises the enveloppe, and allocates the first cache.
  ///
  void initialise(mvm::BumpPtrAllocator& Alloc, const UTF8* name,
                  Signdef* sign) {
    allocator = &Alloc;
    firstCache = &bootCache;
    methodName = name;
    methodSign = sign;
  }

};

} // end namespace jnjvm

#endif
