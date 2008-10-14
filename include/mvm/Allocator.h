//===----------- Allocator.h - A memory allocator  ------------------------===//
//
//                        The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_ALLOCATOR_H
#define MVM_ALLOCATOR_H

#include <cstdlib>
#include <limits>

#include "llvm/Support/Allocator.h"

#include "MvmGC.h"
#include "mvm/Threads/Locks.h"

#ifdef MULTIPLE_GC
#define allocator_new(alloc, cl) collector_new(cl, alloc.GC)
#else
#define allocator_new(alloc, cl) gc_new(cl)
#endif

namespace mvm {

class Allocator {
private:
#ifdef MULTIPLE_GC
  Collector* GC;
#endif

public:
  
#ifndef MULTIPLE_GC
  void* allocateManagedObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT);
  }
#else
  void* allocateManagedObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT, GC);
  }
#endif
  
  void* allocatePermanentMemory(unsigned int sz) {
    return malloc(sz); 
  }
  
  void freePermanentMemory(void* obj) {
    return free(obj); 
  }
  
  void* allocateTemporaryMemory(unsigned int sz) {
    return malloc(sz); 
  }
  
  void freeTemporaryMemory(void* obj) {
    return free(obj); 
  }
};

class BumpPtrAllocator {
private:
  LockNormal TheLock;
  llvm::BumpPtrAllocator Allocator;
public:
  void* Allocate(size_t sz) {
    TheLock.lock();
    void* res = Allocator.Allocate(sz, sz % 4 ? sizeof(void*) : 1);
    TheLock.unlock();
    return res;
  }

  void Deallocate(void* obj) {}

  static void* ThreadAllocate(size_t sz);
  static void* VMAllocate(size_t sz);
};

template <class T>
class STLVMAllocator {
public:
  // type definitions
  typedef T        value_type;
  typedef T*       pointer;
  typedef const T* const_pointer;
  typedef T&       reference;
  typedef const T& const_reference;
  typedef std::size_t    size_type;
  typedef std::ptrdiff_t difference_type;

  // rebind allocator to type U
  template <class U>
  struct rebind {
    typedef STLVMAllocator<U> other;
  };

  // return address of values
  pointer address (reference value) const {
    return &value;
  }
  const_pointer address (const_reference value) const {
    return &value;
  }

  
  STLVMAllocator() {}
  template<class T2> STLVMAllocator(STLVMAllocator<T2>&) {}
  
  ~STLVMAllocator() {}

  size_type max_size () const {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate (size_type num, const void* = 0) { 
    pointer ret = (pointer)mvm::BumpPtrAllocator::VMAllocate(num*sizeof(T));
    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct (pointer p, const T& value) {
    // initialize memory with placement new
    new((void*)p)T(value);
  }

  // destroy elements of initialized storage p
  void destroy (pointer p) {
    // destroy objects by calling their destructor
    p->~T();
  }

  // deallocate storage p of deleted elements
  void deallocate (pointer p, size_type num) {}
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const STLVMAllocator<T1>&,
                 const STLVMAllocator<T2>&) {
  return true;
}

template <class T1, class T2>
bool operator!= (const STLVMAllocator<T1>&,
                 const STLVMAllocator<T2>&) {
  return false;
}

template <class T>
class STLThreadAllocator {
public:
  // type definitions
  typedef T        value_type;
  typedef T*       pointer;
  typedef const T* const_pointer;
  typedef T&       reference;
  typedef const T& const_reference;
  typedef std::size_t    size_type;
  typedef std::ptrdiff_t difference_type;

  // rebind allocator to type U
  template <class U>
  struct rebind {
    typedef STLThreadAllocator<U> other;
  };

  // return address of values
  pointer address (reference value) const {
    return &value;
  }
  const_pointer address (const_reference value) const {
    return &value;
  }

  
  STLThreadAllocator() {}
  template<class T2> STLThreadAllocator(STLThreadAllocator<T2>&) {}
  
  ~STLThreadAllocator() {}

  size_type max_size () const {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate (size_type num, const void* = 0) {
    pointer ret = (pointer)mvm::BumpPtrAllocator::ThreadAllocate(num*sizeof(T));
    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct (pointer p, const T& value) {
    // initialize memory with placement new
    new((void*)p)T(value);
  }

  // destroy elements of initialized storage p
  void destroy (pointer p) {
    // destroy objects by calling their destructor
    p->~T();
  }

  // deallocate storage p of deleted elements
  void deallocate (pointer p, size_type num) {}
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const STLThreadAllocator<T1>&,
                 const STLThreadAllocator<T2>&) {
  return true;
}

template <class T1, class T2>
bool operator!= (const STLThreadAllocator<T1>&,
                 const STLThreadAllocator<T2>&) {
  return false;
}



class PermanentObject {
public:
  void* operator new(size_t sz, BumpPtrAllocator& allocator) {
    return allocator.Allocate(sz);
  }

  void operator delete(void* ptr) {
    free(ptr);
  }
};

} // end namespace mvm

#endif // MVM_ALLOCATOR_H
