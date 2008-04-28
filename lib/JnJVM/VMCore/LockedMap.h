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

struct ltutf8
{
#ifdef MULTIPLE_VM
  bool operator()(const UTF8* s1, const UTF8* s2) const
  {
    if (s1->size < s2->size) return true;
    else if (s1->size > s2->size) return false;
    else return memcmp((const char*)s1->elements, (const char*)s2->elements, 
                       s1->size * sizeof(uint16)) < 0;
  }
#else
  bool operator()(const UTF8* s1, const UTF8* s2) const
  {
    return s1 < s2;
  }
#endif
};

template<class Key, class Container, class Compare>
class LockedMap : public mvm::Object {
public:
  typedef typename std::map<Key, Container, Compare>::iterator iterator;
  typedef Container (*funcCreate)(Key& V, Jnjvm *vm);

  mvm::Lock* lock;
  std::map<Key, Container, Compare,
           gc_allocator<std::pair<Key, Container> > > map;
  
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
  
  inline void remove(Key V) {
    lock->lock();
    map.erase(V);
    lock->unlock();
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
  std::multimap<uint32, const UTF8*, std::less<uint32>,
                gc_allocator< std::pair<uint32, const UTF8*> > > map;
  static VirtualTable* VT;
  const UTF8* lookupOrCreateAsciiz(Jnjvm* vm, const char* asciiz); 
  const UTF8* lookupOrCreateReader(Jnjvm* vm, const uint16* buf, uint32 size);
  const UTF8* lookupAsciiz(const char* asciiz); 
  const UTF8* lookupReader(const uint16* buf, uint32 size);
  
  virtual void TRACER;

  virtual void print(mvm::PrintBuffer* buf) {
    buf->write("UTF8 Hashtable<>");
  }
  
  UTF8Map() {
    lock = mvm::Lock::allocNormal();
  }

  void copy(UTF8Map* newMap) {
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      newMap->map.insert(*i);
    }
  }
  
  void replace(const UTF8* oldUTF8, const UTF8* newUTF8);
  void insert(const UTF8* val);
};

class FieldCmp {
public:
  const UTF8* name;
  CommonClass* classDef;
  const UTF8* type;
  uint32_t access;

  FieldCmp(const UTF8* n, CommonClass* c, const UTF8* t, uint32 a) : name(n), classDef(c), 
    type(t), access(a) {}
  
  inline bool operator<(const FieldCmp &cmp) const {
    if (name < cmp.name) return true;
    else if (name > cmp.name) return false;
    else if (classDef < cmp.classDef) return true;
    else if (classDef > cmp.classDef) return false;
    else return type < cmp.type;
  }
};

class ClassMap : 
    public LockedMap<const UTF8*, CommonClass*, ltutf8 > {
public:
  static VirtualTable* VT;
  
  ClassMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;
};

class FieldMap :
    public LockedMap<FieldCmp, JavaField*, std::less<FieldCmp> > {
public:
  static VirtualTable* VT;
  
  FieldMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;
};

class MethodMap :
    public LockedMap<FieldCmp, JavaMethod*, std::less<FieldCmp> > {
public:
  static VirtualTable* VT;
  
  MethodMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;
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
  
  ZipFileMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;
};

class StringMap :
    public LockedMap<const UTF8*, JavaString*, ltutf8 > {
public:
  static VirtualTable* VT;
  
  StringMap() {
    lock = mvm::Lock::allocRecursive();
  }
  
  virtual void TRACER;
};

class FunctionMap :
    public LockedMap<llvm::Function*, std::pair<Class*, uint32>*, std::less<llvm::Function*> > { 
public:
  static VirtualTable* VT; 
  
  FunctionMap() {
    lock = mvm::Lock::allocNormal();
  }

  virtual void TRACER;
};

class FunctionDefMap :
    public LockedMap<llvm::Function*, JavaMethod*, std::less<llvm::Function*> > { 
public:
  static VirtualTable* VT; 
  
  FunctionDefMap() {
    lock = mvm::Lock::allocNormal();
  }

  virtual void TRACER;
};

class TypeMap :
    public LockedMap<const UTF8*, Typedef*, ltutf8 > {
public:
  static VirtualTable* VT;
  
  inline Typedef* lookupOrCreate(const UTF8*& V, Jnjvm *vm, funcCreate func) {
    assert(0);
    return 0;
  }
  
  TypeMap() {
    lock = mvm::Lock::allocRecursive();
  }
  
  virtual void TRACER;
    
};

class StaticInstanceMap :
    public LockedMap<Class*, std::pair<JavaState, JavaObject*>*, std::less<Class*> > {
public:
  static VirtualTable* VT;
  
  StaticInstanceMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;

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
  
  DelegateeMap() {
    lock = mvm::Lock::allocNormal();
  }
  
  virtual void TRACER;
}; 

} // end namespace jnjvm

#endif
