#ifndef _VMKIT_H_
#define _VMKIT_H_

#include <vector>
#include "mvm/Allocator.h"
#include "mvm/Threads/CollectionRV.h"
#include "mvm/VirtualMachine.h"

namespace mvm {

class VMKit : public mvm::PermanentObject {
public:
  /// allocator - Bump pointer allocator to allocate permanent memory of VMKit
  mvm::BumpPtrAllocator& allocator;

  VMKit(mvm::BumpPtrAllocator &Alloc) : allocator(Alloc) {
	}
	/// ------------------------------------------------- ///
	/// ---             vm managment                  --- ///
	/// ------------------------------------------------- ///
	mvm::SpinLock                lockvms;  // lock for vms
	std::vector<VirtualMachine*> vms;      // array of vms

	size_t addVM(VirtualMachine* vm);
	void   removeVM(size_t id);

	/// ------------------------------------------------- ///
	/// ---             thread managment              --- ///
	/// ------------------------------------------------- ///

  /// rendezvous - The rendezvous implementation for garbage collection.
  ///
#ifdef WITH_LLVM_GCC
  CooperativeCollectionRV rendezvous;
#else
  UncooperativeCollectionRV rendezvous;
#endif

};

}

#endif
