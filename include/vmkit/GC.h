//===----------- GC.h - Garbage Collection Interface -----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef VMKIT_GC_H
#define VMKIT_GC_H

#include <stdint.h>
#include "vmkit/System.h"

class gc;

class gcHeader {
public:
	word_t _header;
	inline gc* toReference() { return (gc*)((uintptr_t)this + hiddenHeaderSize()); }
	static inline size_t hiddenHeaderSize() { return sizeof(gcHeader); }
};

class gcRoot {
public:
  word_t& header(){return toHeader()->_header; }
  inline gcHeader* toHeader() { return (gcHeader*)((uintptr_t)this - gcHeader::hiddenHeaderSize()); }
};

namespace vmkit {
  // TODO(ngeoffray): Make these two constants easily configurable. For now they
  // work for all our supported GCs.
  static const uint32_t GCBits = 8;
  static const bool MovesObject = true;

  static const uint32_t HashBits = 8;
  static const uint64_t GCBitMask = ((1 << GCBits) - 1);
}

#endif
