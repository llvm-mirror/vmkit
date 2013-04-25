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

class VirtualTable;
class gc;

class gcHeader {
public:
	word_t _header;

	inline gc* toReference() { return (gc*)((uintptr_t)this + hiddenHeaderSize()); }

	static inline size_t hiddenHeaderSize() { return sizeof(gcHeader); }
};

class gcRoot {
	private:
		word_t _header;
public:
  virtual           ~gcRoot() {}
  virtual void      tracer(word_t closure) {}

  word_t& header(){return _header; }
  
  inline gcHeader* toHeader() { return (gcHeader*)((uintptr_t)this - gcHeader::hiddenHeaderSize()); }

  /// getVirtualTable - Returns the virtual table of this object.
  ///
  VirtualTable* getVirtualTable() const {
    return ((VirtualTable**)(this))[0];
  }
  
  /// setVirtualTable - Sets the virtual table of this object.
  ///
  void setVirtualTable(VirtualTable* VT) {
    ((VirtualTable**)(this))[0] = VT;
  }
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
