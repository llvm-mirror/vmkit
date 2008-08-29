//===------- SharedMaps.h - Maps for the shared class loader --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "LockedMap.h"

#ifndef ISOLATE_JNJVM_SHARED_MAPS_H
#define ISOLATE_JNJVM_SHARED_MAPS_H

namespace jnjvm {


struct ltarray {
 bool operator()(const ArrayUInt8* a1, const ArrayUInt8* a2) {
  if (a1->size < a2->size) return true;
    else if (a1->size > a2->size) return false;
    else return memcmp((const char*)a1->elements, (const char*)a2->elements, 
                       a1->size * sizeof(uint8)) < 0;
 }
};


class SharedClassByteMap : 
    public LockedMap<const ArrayUInt8*, Class*, ltarray, JnjvmClassLoader* > {
public:
  static VirtualTable* VT;
  
  SharedClassByteMap() {
    lock = mvm::Lock::allocNormal();
  }

  ~SharedClassByteMap() {
    delete lock;
  }
  
  virtual void TRACER;
};


class SharedClassNameMap : 
    public LockedMap<const UTF8*, CommonClass*, ltutf8, JnjvmClassLoader* > {
public:
  
  static VirtualTable* VT;
  
  SharedClassNameMap() {
    lock = mvm::Lock::allocNormal();
  }

  ~SharedClassNameMap() {
    delete lock;
  }
  
  virtual void TRACER;

};

} // end namespace jnjvm

#endif //ISOLATE_JNJVM_SHARED_MAPS_H
