//===----- CLIString.h - Internal correspondance with CLI Strings ---------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_CLI_STRING_H
#define N3_CLI_STRING_H

#include "llvm/GlobalVariable.h"

#include "types.h"
#include "mvm/PrintBuffer.h"

#include "VMObject.h"

namespace n3 {

class UTF8;
class N3;

class CLIString : public VMObject {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("CLI string");
  }
  virtual void tracer(size_t sz);
  
  // !!! pnetlib layout !!!
  sint32 capacity;
  sint32 length;
  uint8 firstChar;
  const UTF8* value;
  llvm::GlobalVariable* _llvmVar;
  llvm::GlobalVariable* llvmVar();

  
  static CLIString* stringDup(const UTF8*& utf8, N3* vm);
  char* strToAsciiz();
  const UTF8* strToUTF8(N3* vm);

};

} // end namespace jnjvm

#endif
