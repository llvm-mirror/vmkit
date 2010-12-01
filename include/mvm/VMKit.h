#ifndef _VMKIT_H_
#define _VMKIT_H_

#include "mvm/Allocator.h"

namespace mvm {

class VMKit : public mvm::PermanentObject {
public:
  /// allocator - Bump pointer allocator to allocate permanent memory of VMKit
  ///
  mvm::BumpPtrAllocator& allocator;

  VMKit(mvm::BumpPtrAllocator &Alloc) : allocator(Alloc) {}
};

}

#endif
