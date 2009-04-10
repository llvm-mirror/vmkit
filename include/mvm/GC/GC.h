//===----------- GC.h - Garbage Collection Interface -----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_GC_H
#define MVM_GC_H

#include <stdint.h>

typedef void (*gc_lock_recovery_fct_t)(int, int, int, int, int, int, int, int);

struct VirtualTable {
  uintptr_t destructor;
  uintptr_t operatorDelete;
  uintptr_t tracer;

  uintptr_t* getFunctions() {
    return &destructor;
  }
};

class gcRoot {
public:
  virtual           ~gcRoot() {}
#ifdef MULTIPLE_GC
  virtual void      tracer(void* GC) {}
#else
  virtual void      tracer(void) {}
#endif
  
  /// getVirtualTable - Returns the virtual table of this object.
  ///
  VirtualTable* getVirtualTable() const {
    return ((VirtualTable**)(this))[0];
  }
  
  /// setVirtualTable - Sets the virtual table of this object.
  ///
  void setVirtualTable(VirtualTable* VT) {
    ((VirtualTable**)(this))[0] = VT;
  }
};

typedef void (*destructor_t)(void*);

class gc_header {
public:
  VirtualTable *_XXX_vt;
  inline gcRoot *_2gc() { return (gcRoot *)this; }
  destructor_t getDestructor() {
    return ((destructor_t*)(this->_XXX_vt))[0];
  }
};

#endif
