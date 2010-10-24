//===------- LockedMap.h - A thread-safe map implementation ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines thread-safe maps that must be deallocated by the owning
// object. For example a class loader is responsible for deallocating the
// types stored in a TypeMap.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_LOCKED_MAP_H
#define JNJVM_LOCKED_MAP_H

#include <map>

#include <cstring>

#include "types.h"

#include "mvm/Allocator.h"
#include "mvm/Threads/Locks.h"
#include "UTF8.h"

#include "JavaArray.h" // for comparing UTF8s

namespace j3 {

class JavaString;
class JnjvmClassLoader;
class Signdef;
class Typedef;
class UserCommonClass;
class UserClassArray;

struct ltutf8
{
  bool operator()(const UTF8* s1, const UTF8* s2) const
  {
    return s1->lessThan(s2);
  }
};

struct ltarray16
{
  bool operator()(const ArrayUInt16* s1, const ArrayUInt16* s2) const
  {
    llvm_gcroot(s1, 0);
    llvm_gcroot(s2, 0);
    if (ArrayUInt16::getSize(s1) < ArrayUInt16::getSize(s2)) return true;
    else if (ArrayUInt16::getSize(s1) > ArrayUInt16::getSize(s2)) return false;
    else return memcmp((const char*)ArrayUInt16::getElements(s1),
                       (const char*)ArrayUInt16::getElements(s2),
                       ArrayUInt16::getSize(s1) * sizeof(uint16)) < 0;
  }
};

  class MapNoGC {
  public:
    static void gcroot(void* val, void* unused) 
      __attribute__ ((always_inline)) {}

  };  
  
  class MapWithGC {
  public:
    static void gcroot(void* val, void* unused) 
      __attribute__ ((always_inline)) {
      llvm_gcroot(val, unused);
    }   
    
  };  


template<class Key, class Container, class Compare, class Meta, class TLock,
         class IsGC>
class LockedMap : public mvm::PermanentObject {
public:
  typedef typename std::map<const Key, Container, Compare>::iterator iterator;
  typedef Container (*funcCreate)(Key& V, Meta meta);

  TLock lock;
  std::map<const Key, Container, Compare,
           gc_allocator<std::pair<const Key, Container> > > map;
  
  inline Container lookupOrCreate(Key& V, Meta meta, funcCreate func) {
    Container res = 0;
    IsGC::gcroot(res, 0);
    IsGC::gcroot((void*)V, 0);
    lock.lock();
    iterator End = map.end();
    iterator I = map.find(V);
    if (I == End) {
      res = func(V, meta);
      map.insert(std::make_pair(V, res));
      lock.unlock();
      return res;
    } else {
      lock.unlock();
      return ((Container)(I->second));
    }
  }
  
  inline void remove(Key V) {
    IsGC::gcroot(V, 0);
    lock.lock();
    map.erase(V);
    lock.unlock();
  }
  
  inline void remove(Key V, Container C) {
    IsGC::gcroot(C, 0);
    IsGC::gcroot(V, 0);
    lock.lock();
    removeUnlocked(V, C); 
    lock.unlock();
  }
  
  inline void removeUnlocked(Key V, Container C) {
    IsGC::gcroot(C, 0);
    IsGC::gcroot((void*)V, 0);
    iterator End = map.end();
    iterator I = map.find(V);
    
    if (I != End && I->second == C)
        map.erase(I); 
  }

  inline Container lookup(Key V) {
    IsGC::gcroot((void*)V, 0);
    lock.lock();
    iterator End = map.end();
    iterator I = map.find(V);
    lock.unlock();
    return I != End ? ((Container)(I->second)) : 0; 
  }

  inline void hash(Key k, Container c) {
    IsGC::gcroot(c, 0);
    IsGC::gcroot(k, 0);
    lock.lock();
    map.insert(std::make_pair(k, c));
    lock.unlock();
  }

  ~LockedMap() {}
};

class ClassMap : 
  public LockedMap<const UTF8*, UserCommonClass*, ltutf8, JnjvmClassLoader*,
                   mvm::LockRecursive, MapNoGC > {

#ifdef USE_GC_BOEHM
public:
  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator) {
    return GC_MALLOC(sz);
  }
#endif
};

class StringMap :
  public LockedMap<const ArrayUInt16*, JavaString*, ltarray16, Jnjvm*,
                   mvm::LockNormal, MapWithGC> {

public:
  void insert(JavaString* str);

};

class TypeMap : public mvm::PermanentObject {
public:
  mvm::LockNormal lock;
  
  std::map<const UTF8*, Typedef*, ltutf8> map;
  typedef std::map<const UTF8*, Typedef*, ltutf8>::iterator iterator;
  
  inline Typedef* lookup(const UTF8* V) {
    iterator End = map.end();
    iterator I = map.find(V);
    return I != End ? I->second : 0; 
  }

  inline void hash(const UTF8* k, Typedef* c) {
    map.insert(std::make_pair(k, c));
  }
};

class SignMap : public mvm::PermanentObject {
public:
  mvm::LockNormal lock;
  
  std::map<const UTF8*, Signdef*, ltutf8> map;
  typedef std::map<const UTF8*, Signdef*, ltutf8>::iterator iterator;
  
  inline Signdef* lookup(const UTF8* V) {
    iterator End = map.end();
    iterator I = map.find(V);
    return I != End ? I->second : 0; 
  }

  inline void hash(const UTF8* k, Signdef* c) {
    map.insert(std::make_pair(k, c));
  }
  
};

} // end namespace j3

#endif
