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

class N3;
class ArrayChar;

class CLIString : public VMObject {
	CLIString() {}
public:
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("CLI string");
  }
  
  static llvm::GlobalVariable* llvmVar(CLIString *self);

  
  static CLIString* stringDup(const ArrayChar* array, N3* vm);
};

} // end namespace jnjvm

#endif
