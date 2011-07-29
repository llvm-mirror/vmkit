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

#include "llvm/ADT/DenseMap.h"

#include <map>

#include <cstring>

#include "types.h"

#include "mvm/Allocator.h"
#include "mvm/Threads/Locks.h"
#include "UTF8.h"

namespace llvm {
// Provide DenseMapInfo for UTF8.
template<>
struct DenseMapInfo<const j3::UTF8*> {
  static inline const j3::UTF8* getEmptyKey() {
    static j3::UTF8 emptyKey(0);
    return &emptyKey;
  }
  static inline const j3::UTF8* getTombstoneKey() {
    static j3::UTF8 tombstoneKey(0);
    return &tombstoneKey;
  }
  static unsigned getHashValue(const j3::UTF8* PtrVal) {
    return PtrVal->hash();
  }
  static bool isEqual(const j3::UTF8* LHS, const j3::UTF8* RHS) { return LHS->equals(RHS); }
};
}

namespace j3 {

class ArrayUInt16;
class JavaString;
class JnjvmClassLoader;
class Signdef;
class Typedef;
class UserCommonClass;
class UserClassArray;

struct ltarray16 {
  bool operator()(const ArrayUInt16* s1, const ArrayUInt16* s2) const;
};

class StringMap : public mvm::PermanentObject {
public:
  typedef std::map<const ArrayUInt16*, JavaString*, ltarray16>::iterator iterator;
  typedef JavaString* (*funcCreate)(const ArrayUInt16*& V, Jnjvm* vm);

  mvm::LockNormal lock;
  std::map<const ArrayUInt16*, JavaString*, ltarray16,
           std::allocator<std::pair<const ArrayUInt16*, JavaString*> > > map;
  
  inline JavaString* lookupOrCreate(const ArrayUInt16* array, Jnjvm* vm, funcCreate func) {
    JavaString* res = 0;
    llvm_gcroot(res, 0);
    llvm_gcroot(array, 0);
    lock.lock();
    iterator End = map.end();
    iterator I = map.find(array);
    if (I == End) {
      res = func(array, vm);
      map.insert(std::make_pair(array, res));
      lock.unlock();
      return res;
    } else {
      lock.unlock();
      return ((JavaString*)(I->second));
    }
  }
  
  inline void remove(const ArrayUInt16* array) {
    llvm_gcroot(array, 0);
    lock.lock();
    map.erase(array);
    lock.unlock();
  }
  
  inline JavaString* lookup(const ArrayUInt16* array) {
    llvm_gcroot(array, 0);
    lock.lock();
    iterator End = map.end();
    iterator I = map.find(array);
    lock.unlock();
    return I != End ? ((JavaString*)(I->second)) : 0; 
  }

  inline void hash(const ArrayUInt16* array, JavaString* str) {
    llvm_gcroot(array, 0);
    llvm_gcroot(str, 0);
    lock.lock();
    map.insert(std::make_pair(array, str));
    lock.unlock();
  }

  inline void removeUnlocked(const ArrayUInt16* array, JavaString* str) {
    llvm_gcroot(str, 0);
    llvm_gcroot(array, 0);
    iterator End = map.end();
    iterator I = map.find(array);
    
    if (I != End && I->second == str) map.erase(I); 
  }

  ~StringMap() {}

  void insert(JavaString* str);
};


class ClassMap : public mvm::PermanentObject {
public:
  mvm::LockRecursive lock;
  llvm::DenseMap<const UTF8*, UserCommonClass*> map;
  typedef llvm::DenseMap<const UTF8*, UserCommonClass*>::iterator iterator;
};

class TypeMap : public mvm::PermanentObject {
public:
  mvm::LockNormal lock;
  llvm::DenseMap<const UTF8*, Typedef*> map;
  typedef llvm::DenseMap<const UTF8*, Typedef*>::iterator iterator;
};

class SignMap : public mvm::PermanentObject {
public:
  mvm::LockNormal lock;
  llvm::DenseMap<const UTF8*, Signdef*> map;
  typedef llvm::DenseMap<const UTF8*, Signdef*>::iterator iterator;
};

} // end namespace j3

#endif
