//===----- ObjectHeader.h - Macros for describing an object header --------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_OBJECT_HEADER_H
#define MVM_OBJECT_HEADER_H

#include <stdint.h>

namespace mvm {
  #if (__WORDSIZE == 64)
  static const uint64_t FatMask = 0x8000000000000000;
#else
  static const uint64_t FatMask = 0x80000000;
#endif

  static const uint64_t ThinCountMask = 0xFF000;
  static const uint64_t ThinCountShift = 12;
  static const uint64_t ThinCountAdd = 0x1000;
  // Mask for all GC objects.
  static const uint64_t NonLockBitsMask = 0xFFF;
  // Mask for the hash code bits.
  static const uint64_t HashMask = 0xFFC;
  // Mask for the GC bits.
  static const uint64_t GCBitMask = 0x3;

  static const bool MovesObject = false;
}

#endif // MVM_OBJECT_HEADER_H
