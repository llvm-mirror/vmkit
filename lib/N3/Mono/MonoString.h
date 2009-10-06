//===---------- MonoString.h - String representation in Mono --------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_MONO_STRING_H
#define N3_MONO_STRING_H

#include "llvm/GlobalVariable.h"

#include "types.h"
#include "mvm/PrintBuffer.h"

#include "CLIString.h"

namespace n3 {

class UTF8;

class MonoString : public CLIString {
public:
  
  // !!! mono layout !!!
  sint32 length;
  uint8 startChar;
  const ArrayUInt16* value;
  llvm::GlobalVariable* _llvmVar;

};

} // end namespace n3

#endif
