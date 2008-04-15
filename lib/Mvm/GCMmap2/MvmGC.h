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

class Collector;

class gc : public gcRoot {
public:
  
  void    markAndTrace() const;
  size_t  objectSize() const;
  void *  operator new(size_t sz, VirtualTable *VT);
  void *  operator new(size_t sz);
  void    operator delete(void *);
  void *  realloc(size_t n);

#ifdef MULTIPLE_GC
#define collector_new(Class, Collector) __gc_new(Class::VT, Collector) Class
  void    markAndTrace(Collector* GC) const;
  size_t  objectSize(Collector* GC) const;
  void *  operator new(size_t sz, VirtualTable *VT, Collector* GC);
  void *  operator new(size_t sz, Collector* GC);
  void    operator delete(void *, Collector* GC);
  void *  realloc(size_t n, Collector* GC);
#endif

};

#ifdef MULTIPLE_GC
#define STATIC
#else
#define STATIC static
#endif

class Collector {
public:

  typedef void (*markerFn)(void);
  
  static void  initialise(markerFn mark, void *base_sp);
  STATIC void  destroy();

  STATIC void           die_if_sigsegv_occured_during_collection(void *addr);
  STATIC int            isStable(gc_lock_recovery_fct_t, int, int, int, int,
                                 int, int, int, int);
  STATIC unsigned int   enable(unsigned int n);
  STATIC void           gcStats(size_t &no, size_t &nbb);
  STATIC void           maybeCollect();
  STATIC void           collect(void);
  STATIC void           inject_my_thread(void *sp);
  STATIC void           remove_my_thread();
  static Collector*     allocate();

  STATIC gc             *begOf(const void *o);
  STATIC int            byteOffset(void *o);
  inline STATIC bool    isObject(const void *o) { return begOf((void*)o); }
        STATIC void     applyFunc(void (*func)(gcRoot *o, void *data), void *data);
        STATIC void     registerMemoryError(void (*func)(unsigned int));
        STATIC int      getMaxMemory(void);
        STATIC int      getFreeMemory(void);
        STATIC int      getTotalMemory(void);
        STATIC void     setMaxMemory(size_t);
        STATIC void     setMinMemory(size_t);
};


#endif
