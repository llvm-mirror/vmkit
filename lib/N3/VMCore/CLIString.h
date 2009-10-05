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
class ArrayUInt16;

class CLIString : public VMObject {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("CLI string");
  }
  virtual void TRACER;
  
  llvm::GlobalVariable* llvmVar();

  
  static CLIString* stringDup(const UTF8*& utf8, N3* vm);
  const  UTF8* strToUTF8(N3* vm);
	ArrayUInt16 *strToArray(N3 *vm) const;
};

} // end namespace jnjvm

#endif
