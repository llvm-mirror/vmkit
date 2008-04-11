//===---------- Object.h - Common layout for all objects ------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_OBJECT_H
#define MVM_OBJECT_H

#include <assert.h>

#include "types.h"
#include "MvmGC.h"

namespace mvm {


#define VT_DESTRUCTOR_OFFSET 0
#define VT_GC_DESTRUCTOR_OFFSET 1
#define VT_DESTROYER_OFFSET 2
#define VT_TRACER_OFFSET 3
#define VT_PRINT_OFFSET 4
#define VT_HASHCODE_OFFSET 5
#define VT_SIZE 24


#define GC_defass(TYPE, name)                                  \
  TYPE    *_hidden_##name;                                     \
	inline TYPE    *name()        { return _hidden_##name; }     \
	inline TYPE    *name(TYPE *v) { return _hidden_##name = v; }

class PrintBuffer;
class Object;

class Object : public gc {
public:
  static VirtualTable* VT; 
  
  VirtualTable* getVirtualTable() const {
    return ((VirtualTable**)(this))[0];
  }
  
  void setVirtualTable(VirtualTable* VT) {
    ((VirtualTable**)(this))[0] = VT;
  }

  char *printString(void) const;

#if !defined(GC_GEN)
  inline gc *gcset(void *ptr, gc *src) { return *((gc **)ptr) = src; }
#endif
  
  virtual void      destroyer(size_t) {}
  virtual void      tracer(size_t) {}
  virtual void      print(PrintBuffer *buf) const;
  virtual intptr_t  hashCode(){ return (intptr_t)this;}

protected:
  static Object **rootTable;
  static int	   rootTableSize, rootTableLimit;

  static void growRootTable(void);

public:
  
  inline static void pushRoot(Object *obj) {
    if (rootTableSize >= rootTableLimit)
      growRootTable();
    rootTable[rootTableSize++]= obj;
  }

  inline static void pushRoot(Object &var) {
    pushRoot(var);
  }

  inline static Object *popRoots(size_t nRoots) {
    rootTableSize-= nRoots;
    assert(rootTableSize >= 0);
    return rootTable[rootTableSize];
  }

  inline static Object *popRoot(void) {
    return popRoots(1);
  }

  static void markAndTraceRoots(void);
  static void initialise();

};

} // end namespace mvm

#endif // MVM_OBJECT_H
