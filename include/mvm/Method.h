//===-------------- Method.h - Collectable methods ------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_METHOD_H
#define MVM_METHOD_H

extern "C" void __deregister_frame(void*);

namespace mvm {

class Code {
public:

  Code() {
    stubSize = 0;
    FunctionStart = 0;
    FunctionEnd = 0;
    stub = 0;
    frameRegister = 0;
    exceptionTable = 0;
    metaInfo = 0;
  }

  unsigned stubSize;
  unsigned char* FunctionStart;
  unsigned char* FunctionEnd;
  unsigned char* stub;
  unsigned char* frameRegister;
  unsigned char* exceptionTable;

  void* metaInfo;

  void* getMetaInfo() { return metaInfo; }
  void  setMetaInfo(void* m) { metaInfo = m; }

  inline void unregisterFrame() {
    __deregister_frame((unsigned char*)frameRegister);
  }
};

} // end namespace mvm

#endif // MVM_METHOD_H
