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
#include "types.h"

#define gc_allocator std::allocator
#define gc_new(Class)  __gc_new(Class::VT) Class
#define __gc_new new


#ifdef MULTIPLE_GC
#define STATIC_TRACER(type) staticTracer(type* obj, void* GC)
#define TRACER tracer(void* GC)
#define CALL_TRACER tracer(GC)
#define MARK_AND_TRACE markAndTrace((Collector*)GC)
#else
#define STATIC_TRACER(type) staticTracer(type* obj)
#define TRACER tracer()
#define CALL_TRACER tracer()
#define MARK_AND_TRACE markAndTrace()
#endif

namespace mvm {
  class Thread;
}

class Collector;

class gc : public gcRoot {
public:
 
#ifndef MULTIPLE_GC
  void    markAndTrace() const;
  size_t  objectSize() const;
  void *  operator new(size_t sz, VirtualTable *VT);
  void *  operator new(size_t sz);
  void    operator delete(void *);
  void *  realloc(size_t n);

#else
  void    markAndTrace(Collector* GC) const;
  size_t  objectSize(Collector* GC) const;
  void *  operator new(size_t sz, VirtualTable *VT, Collector* GC);
  void *  operator new(size_t sz, Collector* GC);
  void    operator delete(void *, Collector* GC);
  void *  realloc(size_t n, Collector* GC);
#endif

};

class Collector {
public:
  typedef void (*markerFn)(void*);
  
  static void  initialise(markerFn mark);
  static void  destroy();

  static void           die_if_sigsegv_occured_during_collection(void *addr);
  static int            isStable(gc_lock_recovery_fct_t, int, int, int, int,
                                 int, int, int, int);
  static unsigned int   enable(unsigned int n);
  static void           gcStats(size_t &no, size_t &nbb);
  static void           maybeCollect();
  static void           collect(void);
  static void           inject_my_thread(mvm::Thread* th);
  static void           remove_my_thread(mvm::Thread* th);

  static bool           isLive(void* ptr);
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
