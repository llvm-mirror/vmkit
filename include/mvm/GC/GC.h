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
#include <map>

struct VirtualTable {
  uintptr_t destructor;
  uintptr_t operatorDelete;
  uintptr_t tracer;

  uintptr_t* getFunctions() {
    return &destructor;
  }

  VirtualTable(uintptr_t d, uintptr_t o, uintptr_t t) {
    destructor = d;
    operatorDelete = o;
    tracer = t;
  }

  VirtualTable() {}

  static void emptyTracer(void*) {}
};

class gcRoot {
public:
  virtual           ~gcRoot() {}
  virtual void      tracer(void) {}
  
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

namespace mvm {

class Thread;

class StackScanner {
public:
  virtual void scanStack(mvm::Thread* th) = 0;
  virtual ~StackScanner() {}
};

class UnpreciseStackScanner : public StackScanner {
public:
  virtual void scanStack(mvm::Thread* th);
};

class PreciseStackScanner : public StackScanner {
public:
  virtual void scanStack(mvm::Thread* th);
};

}

#endif
