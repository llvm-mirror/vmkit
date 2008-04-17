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

#include <sys/types.h>

typedef void (*gc_lock_recovery_fct_t)(int, int, int, int, int, int, int, int);

typedef void VirtualTable;

class gcRoot {
public:
  virtual           ~gcRoot() {}
  virtual void      destroyer(size_t) {}
#ifdef MULTIPLE_GC
  virtual void      tracer(void* GC) {}
#else
  virtual void      tracer(void) {}
#endif
};

class gc_header {
public:
  VirtualTable *_XXX_vt;
  inline gcRoot *_2gc() { return (gcRoot *)this; }
};

#endif
