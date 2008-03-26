//===------- LockedMap.h - A thread-safe map implementation ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_LOCKED_MAP_H
#define JNJVM_LOCKED_MAP_H

#include <map>

#include <string.h>

#include "types.h"

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaTypes.h"
#include "JavaString.h"
#include "Zip.h"

namespace jnjvm {

class JavaObject;
class Jnjvm;

template<class Key, class Container, class Compare>
class LockedMap : public mvm::Object {
public:
  typedef typename std::map<Key, Container, Compare>::iterator iterator;
  typedef Container (*funcCreate)(Key& V, Jnjvm *vm);

  mvm::Lock* lock;
  std::map<Key, Container, Compare> map;
  
  inline Container lookupOrCreate(Key& V, Jnjvm *vm, funcCreate func) {
    lock->lock();
    iterator End = map.end();
    iterator I = map.find(V);
    if (I == End) {
      Container res = func(V, vm);
      map.insert(std::make_pair(V, res));
      lock->unlock();
      return res;
    } else {
      lock->unlock();
      return ((Container)(I->second));
    }
  }

  inline Container lookup(Key V) {
    lock->lock();
    iterator End = map.end();
    iterator I = map.find(V);
    lock->unlock();
    return I != End ? ((Container)(I->second)) : 0; 
  }

  inline void hash(Key k, Container c) {
    lock->lock();
    map.insert(std::make_pair(k, c));
    lock->unlock();
  }


  virtual void print(mvm::PrintBuffer* buf) {
    buf->write("Hashtable<>");
  }

};

class UTF8Map : public mvm::Object {
public:
  typedef std::multimap<uint32, const UTF8*>::iterator iterator;
  
  mvm::Lock* lock;
  std::multimap<uint32, const UTF8*> map;
  static VirtualTable* VT;
  const UTF8* lookupOrCreateAsciiz(const char* asciiz); 
  const UTF8* lookupOrCreateReader(const uint16* buf, uint32 size);
  const UTF8* lookupAsciiz(const char* asciiz); 
  const UTF8* lookupReader(const uint16* buf, uint32 size);
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }

  virtual void print(mvm::PrintBuffer* buf) {
    buf->write("UTF8 Hashtable<>");
  }
  
  static UTF8Map* allocate() {
    UTF8Map* map = gc_new(UTF8Map)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }

  UTF8Map* copy() {
    UTF8Map* newMap = allocate();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      newMap->map.insert(*i);
    }
    return newMap;
  }
  
  void replace(const UTF8* oldUTF8, const UTF8* newUTF8);
  void insert(const UTF8* val);
};

class FieldCmp {
public:
  const UTF8* name;
  CommonClass* classDef;
  const UTF8* type;

  FieldCmp(const UTF8* n, CommonClass* c, const UTF8* t) : name(n), classDef(c), 
    type(t) {}
  
  inline bool operator<(const FieldCmp &cmp) const {
    if (name < cmp.name) return true;
    else if (name > cmp.name) return false;
    else if (classDef < cmp.classDef) return true;
    else if (classDef > cmp.classDef) return false;
    else return type < cmp.type;
  }
};

class ClassMap : 
    public LockedMap<const UTF8*, CommonClass*, std::less<const UTF8*> > {
public:
  static VirtualTable* VT;
  static ClassMap* allocate() {
    ClassMap* map = gc_new(ClassMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

class FieldMap :
    public LockedMap<FieldCmp, JavaField*, std::less<FieldCmp> > {
public:
  static VirtualTable* VT;
  static FieldMap* allocate() {
    FieldMap* map = gc_new(FieldMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

class MethodMap :
    public LockedMap<FieldCmp, JavaMethod*, std::less<FieldCmp> > {
public:
  static VirtualTable* VT;
  static MethodMap* allocate() {
    MethodMap* map = gc_new(MethodMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

class ZipFileMap : public LockedMap<const char*, ZipFile*, ltstr> {
public:
  static VirtualTable* VT;
  static ZipFileMap* allocate() {
    ZipFileMap* map = gc_new(ZipFileMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

class StringMap :
    public LockedMap<const UTF8*, JavaString*, std::less<const UTF8*> > {
public:
  static VirtualTable* VT;
  static StringMap* allocate() {
    StringMap* map = gc_new(StringMap)();
    map->lock = mvm::Lock::allocRecursive();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

class FunctionMap :
    public LockedMap<llvm::Function*, std::pair<Class*, uint32>*, std::less<llvm::Function*> > { 
public:
  static VirtualTable* VT; 
  static FunctionMap* allocate() {
    FunctionMap* map = gc_new(FunctionMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }

  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->first->markAndTrace();
    }
  }
};

class FunctionDefMap :
    public LockedMap<llvm::Function*, JavaMethod*, std::less<llvm::Function*> > { 
public:
  static VirtualTable* VT; 
  static FunctionDefMap* allocate() {
    FunctionDefMap* map = gc_new(FunctionDefMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }

  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
};

class TypeMap :
    public LockedMap<const UTF8*, Typedef*, std::less<const UTF8*> > {
public:
  static VirtualTable* VT;
  
  inline Typedef* lookupOrCreate(const UTF8*& V, Jnjvm *vm, funcCreate func) {
    assert(0);
    return 0;
  }
  
  static TypeMap* allocate() {
    TypeMap* map = gc_new(TypeMap)();
    map->lock = mvm::Lock::allocRecursive();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->markAndTrace();
    }
  }
    
};

class StaticInstanceMap :
    public LockedMap<Class*, std::pair<uint8, JavaObject*>*, std::less<Class*> > {
public:
  static VirtualTable* VT;
  
  static StaticInstanceMap* allocate() {
    StaticInstanceMap* map = gc_new(StaticInstanceMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->first->markAndTrace();
      i->second->second->markAndTrace();
    }
  }

  virtual void destroyer(size_t sz) {
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      delete i->second;
    }
  }
}; 

class DelegateeMap :
    public LockedMap<CommonClass*, JavaObject*, std::less<CommonClass*> > {
public:
  static VirtualTable* VT;
  
  static DelegateeMap* allocate() {
    DelegateeMap* map = gc_new(DelegateeMap)();
    map->lock = mvm::Lock::allocNormal();
    return map;
  }
  
  virtual void tracer(size_t sz) {
    //lock->markAndTrace();
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->first->markAndTrace();
      i->second->markAndTrace();
    }
  }
}; 

} // end namespace jnjvm

#endif
