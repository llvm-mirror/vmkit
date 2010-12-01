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
#include <cstring>
#include <limits>

#include "llvm/Support/Allocator.h"

#include "mvm/Threads/Locks.h"

#define declare_gcroot(type, name) type name; llvm_gcroot(name, 0); name

namespace mvm {

class BumpPtrAllocator {
private:
  SpinLock TheLock;
  llvm::BumpPtrAllocator Allocator;
public:
  void* Allocate(size_t sz, const char* name) {
#ifdef USE_GC_BOEHM
    return GC_MALLOC(sz);
#else
    TheLock.acquire();
    void* res = Allocator.Allocate(sz, sizeof(void*));
    TheLock.release();
    memset(res, 0, sz);
    return res;
#endif
  }

  void Deallocate(void* obj) {}

};

class ThreadAllocator {
private:
  llvm::BumpPtrAllocator Allocator;
public:
  void* Allocate(size_t sz) {
#ifdef USE_GC_BOEHM
    return GC_MALLOC(sz);
#else
    void* res = Allocator.Allocate(sz, sizeof(void*));
    memset(res, 0, sz);
    return res;
#endif
  }

  void Deallocate(void* obj) {}

};

class PermanentObject {
public:
  void* operator new(size_t sz, BumpPtrAllocator& allocator,
                     const char* name) {
    return allocator.Allocate(sz, name);
  }
  
  void operator delete(void* ptr) {
    free(ptr);
  }

  void* operator new [](size_t sz, BumpPtrAllocator& allocator,
                        const char* name) {
    return allocator.Allocate(sz, name);
  }
  
  void* operator new[](size_t sz) {
    return malloc(sz);
  }
  
  void operator delete[](void* ptr) {
    return free(ptr);
  }
};

/// JITInfo - This class can be derived from to hold private JIT-specific
/// information. Objects of type are accessed/created with
/// <Class>::getInfo and destroyed when the <Class> object is destroyed.
struct JITInfo : public mvm::PermanentObject {
  virtual ~JITInfo() {}
  virtual void clear() {}
};

} // end namespace mvm

#endif // MVM_ALLOCATOR_H
