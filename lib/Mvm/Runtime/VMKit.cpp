#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"

using namespace mvm;

VMKit::VMKit(mvm::BumpPtrAllocator &Alloc) : allocator(Alloc) {
	vms         = 0;
	numberOfVms = 0;
}

size_t VMKit::addVM(VirtualMachine* vm) {
	vmkitLock.lock();

	for(size_t i=0; i<numberOfVms; i++)
		if(!vms[i]) {
			vms[i] = vm;
			vmkitLock.unlock();
			return i;
		}

	int res = numberOfVms;
	numberOfVms = numberOfVms ? (numberOfVms<<1) : 1;
	// reallocate the vms
	VirtualMachine **newVms = new VirtualMachine*[numberOfVms];
	memcpy(newVms, vms, res*sizeof(VirtualMachine*));
	VirtualMachine **oldVms = vms;
	vms = newVms; // vms must always contain valid data
	delete[] oldVms;
	
 	// reallocate the allVMDatas
 	for(Thread* cur=runningThreads.next(); cur!=&runningThreads; cur=cur->next()) {
		cur->reallocAllVmsData(res, numberOfVms);
	}

	vmkitLock.unlock();

	return res;
}

void VMKit::removeVM(size_t id) {
	vms[id] = 0;
}

void VMKit::addThread(mvm::Thread* th) {
	vmkitLock.lock();
	numberOfRunningThreads++;
	th->appendTo(&runningThreads);
	th->reallocAllVmsData(0, numberOfVms);
	vmkitLock.unlock();
}
  
void VMKit::removeThread(mvm::Thread* th) {
	vmkitLock.lock();
	numberOfRunningThreads--;
	th->remove();
	th->allVmsData = 0;
	vmkitLock.unlock();
}
