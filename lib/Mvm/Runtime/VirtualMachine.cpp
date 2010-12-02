#include "mvm/VirtualMachine.h"
#include "mvm/VMKit.h"

using namespace mvm;

VirtualMachine::VirtualMachine(mvm::BumpPtrAllocator &Alloc, mvm::VMKit* vmk) :	allocator(Alloc) {
	vmkit = vmk;
	vmID = vmkit->addVM(this);
}

VirtualMachine::~VirtualMachine() {
	vmkit->removeVM(vmID);
}
