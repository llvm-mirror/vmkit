//===---------- PNetString.h - String representation in PNet --------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_PNET_STRING_H
#define N3_PNET_STRING_H

#include "llvm/GlobalVariable.h"

#include "types.h"
#include "mvm/PrintBuffer.h"

#include "CLIString.h"

namespace n3 {

class ArrayChar;

class PNetString : public CLIString {
public:
  
  // !!! pnetlib layout !!!
  sint32 capacity;
  sint32 length;
  uint8 firstChar;
  const ArrayChar* value;
  llvm::GlobalVariable* _llvmVar;

};

} // end namespace n3

#endif
