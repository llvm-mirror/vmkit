//===---------- MethodInfo.h - Meta information for methods ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_METHODINFO_H
#define MVM_METHODINFO_H

#include "mvm/Allocator.h"
#include "mvm/GC/GC.h"

namespace mvm {

class MethodInfo : public PermanentObject {
public:
  virtual void print(void* ip, void* addr) = 0;
  virtual void scan(uintptr_t closure, void* ip, void* addr) = 0;
  virtual bool isHighLevelMethod() {
    return false;
  }

  static void* isStub(void* ip, void* addr) {
    bool isStub = ((unsigned char*)ip)[0] == 0xCE;
    if (isStub) ip = ((void**)addr)[2];
    return ip;
  }

  void* MetaInfo;
  void* Owner;
};

class CamlFrame {
public:
  void* ReturnAddress;
  uint16_t FrameSize;
  uint16_t NumLiveOffsets;
  int16_t LiveOffsets[1];
};

class CamlMethodInfo : public MethodInfo {
public:
  CamlFrame* CF;
  virtual void scan(uintptr_t closure, void* ip, void* addr);
  CamlMethodInfo(CamlFrame* C) : CF(C) {
    Owner = NULL;
    MetaInfo = NULL;
  }
};

class StaticCamlMethodInfo : public CamlMethodInfo {
  const char* name;
public:
  virtual void print(void* ip, void* addr);
  StaticCamlMethodInfo(CamlFrame* CF, const char* n) :
    CamlMethodInfo(CF) {
    Owner = NULL;
    MetaInfo = NULL;
    name = n;
  }
};

class DefaultMethodInfo : public MethodInfo {
public:
  virtual void print(void* ip, void* addr);
  virtual void scan(uintptr_t closure, void* ip, void* addr);
  static DefaultMethodInfo DM;
    
  DefaultMethodInfo() {
    Owner = NULL;
    MetaInfo = NULL;
  }
};


} // end namespace mvm
#endif // MVM_METHODINFO_H
