//===------- JavaAllocator.h - A memory allocator for Jnjvm ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_ALLOCATOR_H
#define JNJVM_JAVA_ALLOCATOR_H

#include "MvmGC.h"

#ifdef MULTIPLE_GC
#define allocator_new(alloc, cl) collector_new(cl, alloc.GC)
#else
#define allocator_new(alloc, cl) gc_new(cl)
#endif

namespace jnjvm {

class JavaAllocator {

#ifdef MULTIPLE_GC
  Collector* GC;
#endif

public:
  
#ifndef MULTIPLE_GC
  void* allocateObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT);
  }
#else
  void* allocateObject(unsigned int sz, VirtualTable* VT) {
    return gc::operator new(sz, VT, GC);
  }
#endif

};

} // end namespace jnjvm

#endif // JNJVM_JAVA_ALLOCATOR_H
