//===----------- GC.h - Garbage Collection Interface -----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_MMAP_GC_H
#define MVM_MMAP_GC_H

#include <sys/types.h>
#include "mvm/GC/GC.h"

#define gc_allocator std::allocator
#define gc_new(Class)  __gc_new(Class::VT) Class
#define __gc_new new

class gc : public gcRoot {
public:
  
  void    markAndTrace() const;
  size_t  objectSize() const;
  void *  operator new(size_t sz, VirtualTable *VT);
  void *  operator new(size_t sz);
  void    operator delete(void *);
  void *  realloc(size_t n);

};

class Collector {
public:

  typedef void (*markerFn)(void);
  
  static void  initialise(markerFn mark, void *base_sp);
  static void  destroy();

  static void           die_if_sigsegv_occured_during_collection(void *addr);
  static int            isStable(gc_lock_recovery_fct_t, int, int, int, int,
                                 int, int, int, int);
  static unsigned int   enable(unsigned int n);
  static void           gcStats(size_t &no, size_t &nbb);
  static void           maybeCollect();
  static void           collect(void);
  static void           inject_my_thread(void *sp);
  static void           remove_my_thread();
  static Collector*     allocate();

  static gc             *begOf(const void *o);
  static int            byteOffset(void *o);
  inline static bool    isObject(const void *o) { return begOf((void*)o); }
        static void     applyFunc(void (*func)(gcRoot *o, void *data), void *data);
        static void     registerMemoryError(void (*func)(unsigned int));
        static int      getMaxMemory(void);
        static int      getFreeMemory(void);
        static int      getTotalMemory(void);
        static void     setMaxMemory(size_t);
        static void     setMinMemory(size_t);
};


#endif
