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

#include <dlfcn.h>
#include <stdint.h>
#include <map>


#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif


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

class CamlFrame {
public:
  void* ReturnAddress;
  uint16_t FrameSize;
  uint16_t NumLiveOffsets;
  int16_t LiveOffsets[1];
};

class StaticGCMap {
public:
  std::map<void*, void*> GCInfos;

  StaticGCMap() {
    CamlFrame* currentFrame =
      (CamlFrame*)dlsym(SELF_HANDLE, "camlVmkitoptimized__frametable");
    
    if (currentFrame) {
      while (true) {

        if (!currentFrame->ReturnAddress) break; 
        GCInfos.insert(std::make_pair(currentFrame->ReturnAddress,
                                    currentFrame));
    
        currentFrame = (CamlFrame*) ((char*)currentFrame + 
          (currentFrame->NumLiveOffsets % 2) * sizeof(uint16_t) +
          currentFrame->NumLiveOffsets * sizeof(uint16_t) +
          sizeof(void*) + sizeof(uint16_t) + sizeof(uint16_t));
      }
    }
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

class CamlStackScanner : public StackScanner {
public:
  virtual void scanStack(mvm::Thread* th);
};

}

#endif
