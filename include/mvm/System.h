//===-------------------- System.h - System utils -------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_SYSTEM_H
#define MVM_SYSTEM_H

#include <stdint.h>

namespace mvm {

const int kWordSize = sizeof(intptr_t);
const int kWordSizeLog2 = kWordSize == 4 ? 2 : 3;

class System {
public:
  static bool IsWordAligned(intptr_t ptr) {
    return (ptr & (kWordSize - 1)) == 0;
  }

  static intptr_t WordAlignUp(intptr_t ptr) {
    if (!IsWordAligned(ptr)) {
      return (ptr & ~(kWordSize - 1)) + kWordSize;
    }
    return ptr;
  }
};

}

#endif
