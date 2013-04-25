//===----------- VmkitGC.h - Garbage Collection Interface -------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef VMKIT_MMTK_GC_H
#define VMKIT_MMTK_GC_H

#include "vmkit/GC.h"
#include "vmkit/Locks.h"
#include <cstdlib>

extern "C" void* vmkitgcmallocUnresolved(uint32_t sz, void* type);
extern "C" void* vmkitgcmalloc(uint32_t sz, void* type);

class gc : public gcRoot {
public:

  size_t objectSize() const {
    abort();
    return 0;
  }

  void* operator new(size_t sz, void *type) {
    return vmkitgcmallocUnresolved(sz, type);
  }
};

extern "C" void arrayWriteBarrier(void* ref, void** ptr, void* value);
extern "C" void fieldWriteBarrier(void* ref, void** ptr, void* value);
extern "C" void nonHeapWriteBarrier(void** ptr, void* value);

namespace vmkit {
  
class Collector {
public:
  static int verbose;

  static bool isLive(gc* ptr, word_t closure) __attribute__ ((always_inline)); 
  static void scanObject(void** ptr, word_t closure) __attribute__ ((always_inline));
  static void markAndTrace(void* source, void* ptr, word_t closure) __attribute__ ((always_inline));
  static void markAndTraceRoot(void* source, void* ptr, word_t closure) __attribute__ ((always_inline));
  static gc*  retainForFinalize(gc* val, word_t closure) __attribute__ ((always_inline));
  static gc*  retainReferent(gc* val, word_t closure) __attribute__ ((always_inline));
  static gc*  getForwardedFinalizable(gc* val, word_t closure) __attribute__ ((always_inline));
  static gc*  getForwardedReference(gc* val, word_t closure) __attribute__ ((always_inline));
  static gc*  getForwardedReferent(gc* val, word_t closure) __attribute__ ((always_inline));
  static void objectReferenceWriteBarrier(gc* ref, gc** slot, gc* value) __attribute__ ((always_inline));
  static void objectReferenceArrayWriteBarrier(gc* ref, gc** slot, gc* value) __attribute__ ((always_inline));
  static void objectReferenceNonHeapWriteBarrier(gc** slot, gc* value) __attribute__ ((always_inline));
  static bool objectReferenceTryCASBarrier(gc* ref, gc** slot, gc* old, gc* value) __attribute__ ((always_inline));
  static bool needsWriteBarrier() __attribute__ ((always_inline));
  static bool needsNonHeapWriteBarrier() __attribute__ ((always_inline));

  static void collect();
  
  static void initialise(int argc, char** argv);
  
  static int getMaxMemory() {
    return 0;
  }
  
  static int getFreeMemory() {
    return 0;
  }
  
  static int getTotalMemory() {
    return 0;
  }

  void setMaxMemory(size_t sz){
  }

  void setMinMemory(size_t sz){
  }

  static void* begOf(gc*);
};

}

class VirtualTable;
extern "C" void* VTgcmallocUnresolved(uint32_t sz, void* VT);
extern "C" void* VTgcmalloc(uint32_t sz, void* VT);
extern "C" void EmptyDestructor();

/*
 * C++ VirtualTable data layout representation. This is the base for
 * every object layout based on virtual tables.
 * See at J3 JavaObject.h file, JavaVirtualTable class definition for an example.
 *
 * Note: If you use VirtualTable, your object root must have a virtual destructor
 * and a virtual method called tracer which traces all GC references contained.
 *
 * Here is an exemple of the minimal compatible object class you can have with VirtualTable:
 *
 * class myRoot : public gc {
 *   public:
 *   virtual      ~myRoot() {}
 *   virtual void tracer(word_t closure) {}
 *
 *   void* operator new(size_t sz, VirtualTable *VT) {
 *     return VTgcmallocUnresolved(sz, VT);
 *	 }
 * };
 *
 */
class VirtualTable {
 public:
  word_t destructor;
  word_t operatorDelete;
  word_t tracer;

  static uint32_t numberOfBaseFunctions() {
    return 3;
  }

  static uint32_t numberOfSpecializedTracers() {
    return 0;
  }

  word_t* getFunctions() {
    return &destructor;
  }

  VirtualTable(word_t d, word_t o, word_t t) {
    destructor = d;
    operatorDelete = o;
    tracer = t;
  }

  VirtualTable() {
    destructor = reinterpret_cast<word_t>(EmptyDestructor);
  }

  bool hasDestructor() {
    return destructor != reinterpret_cast<word_t>(EmptyDestructor);
  }

  static void emptyTracer(void*) {}

  /// getVirtualTable - Returns the virtual table of this reference.
  ///
  static const VirtualTable* getVirtualTable(gc* ref) {
    return ((VirtualTable**)(ref))[0];
  }

  /// setVirtualTable - Sets the virtual table of this reference.
  ///
  static void setVirtualTable(gc* ref, VirtualTable* VT) {
    ((VirtualTable**)(ref))[0] = VT;
  }

};

#endif
