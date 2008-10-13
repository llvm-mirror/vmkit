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

#include "MvmGC.h"

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

class PermanentObject {
public:
  void* operator new(size_t sz, Allocator* allocator) {
    return allocator->allocatePermanentMemory(sz);
  }

  void operator delete(void* ptr) {
    free(ptr);
  }
};

} // end namespace mvm

#endif // MVM_ALLOCATOR_H
