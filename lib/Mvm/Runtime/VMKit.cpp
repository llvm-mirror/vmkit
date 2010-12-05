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
 	Thread* cur=oneThread;
	if(cur) {
		do {
			cur->reallocAllVmsData(res, numberOfVms);
			cur = cur->next0();
		} while(cur!=oneThread);
	}

	vmkitLock.unlock();

	return res;
}

void VMKit::removeVM(size_t id) {
	vms[id] = 0;
}

void VMKit::addThread(mvm::Thread* th) {
	vmkitLock.lock();
	numberOfThreads++;
	if (th != oneThread) {
		if (oneThread) th->appendTo(oneThread);
		else oneThread = th;
	}
	th->reallocAllVmsData(0, numberOfVms);
	vmkitLock.unlock();
}
  
void VMKit::removeThread(mvm::Thread* th) {
	vmkitLock.lock();
	numberOfThreads--;
	if (oneThread == th) oneThread = th->next0();
	th->remove();
	if (!numberOfThreads) oneThread = 0;
	delete th->allVmsData;
	th->allVmsData = 0;
	vmkitLock.unlock();
}
