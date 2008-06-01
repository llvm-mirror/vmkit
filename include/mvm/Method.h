//===--------- Method.h - Meta information for methods --------------------===//
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

/// Code - This class contains meta information of a dynamicall generated
/// function.
///
class Code {
private:
  
  /// metaInfo - A meta info field to store virtual machine informations.
  ///
  void* metaInfo;

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
  
  /// stubSize - The size of the stub that will generate the function.
  ///
  unsigned stubSize;
  
  /// FunctionStart - The starting address in memory of the generated function.
  ///
  unsigned char* FunctionStart;

  /// FunctionStart - The ending address in memory of the generated function.
  ///
  unsigned char* FunctionEnd;

  /// stub - The starting address of the stub.
  ///
  unsigned char* stub;

  /// exceptionTable - The exception table of this function.
  ///
  unsigned char* exceptionTable;
  
  /// frameRegister - The address where to register and unregister the
  /// exception table.
  unsigned char* frameRegister;


  /// getMetaInfo - Returns the virtual machine information related to this
  /// method.
  void* getMetaInfo() { return metaInfo; }
  
  /// setMetaInfo - Sets the virtual machine information of this method.
  ///
  void  setMetaInfo(void* m) { metaInfo = m; }

  /// unregisterFrame - Unregisters the exception table of this method.
  ///
  inline void unregisterFrame() {
    __deregister_frame((unsigned char*)frameRegister);
  }
};

} // end namespace mvm

#endif // MVM_METHOD_H
