#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"

using namespace mvm;

VMKit::VMKit(mvm::BumpPtrAllocator &Alloc) : allocator(Alloc) {
	vms          = 0;
	vmsArraySize = 0;
}

void VMKit::startCollection() {
	vmkitLock();
	for(size_t i=0; i<vmsArraySize; i++)
		if(vms[i])
			vms[i]->startCollection();
	vmkitUnlock();
}

void VMKit::endCollection() {
	vmkitLock();
	for(size_t i=0; i<vmsArraySize; i++)
		if(vms[i])
			vms[i]->endCollection();
	vmkitUnlock();
}

size_t VMKit::addVM(VirtualMachine* vm) {
	vmkitLock();

	for(size_t i=0; i<vmsArraySize; i++)
		if(!vms[i]) {
			vms[i] = vm;
			vmkitUnlock();
			return i;
		}

	int res = vmsArraySize;
	vmsArraySize = vmsArraySize ? (vmsArraySize<<1) : 1;
	// reallocate the vms
	VirtualMachine **newVms = new VirtualMachine*[vmsArraySize];

	memcpy(newVms, vms, res*sizeof(VirtualMachine*));
	memset(newVms + res*sizeof(VirtualMachine*), 0, (vmsArraySize-res)*sizeof(VirtualMachine*));
	newVms[res] = vm;

	VirtualMachine **oldVms = vms;
	vms = newVms; // vms must always contain valid data
	delete[] oldVms;
	
 	// reallocate the allVMDatas
 	for(Thread* cur=preparedThreads.next(); cur!=&preparedThreads; cur=cur->next()) {
		cur->reallocAllVmsData(res, vmsArraySize);
	}

 	for(Thread* cur=runningThreads.next(); cur!=&runningThreads; cur=cur->next()) {
		cur->reallocAllVmsData(res, vmsArraySize);
	}

	vmkitUnlock();

	return res;
}

void VMKit::removeVM(size_t id) {
	// what can I do with the VMThreadData?
	vms[id] = 0;
}

void VMKit::registerPreparedThread(mvm::Thread* th) {
	vmkitLock();
	th->appendTo(&preparedThreads);
	th->reallocAllVmsData(0, vmsArraySize);
	vmkitUnlock();
}
  
void VMKit::unregisterPreparedThread(mvm::Thread* th) {
	vmkitLock();
	th->remove();
	for(size_t i=0; i<vmsArraySize; i++)
		if(th->allVmsData[i])
			delete th->allVmsData[i];
	delete th->allVmsData;
	vmkitUnlock();
}

void VMKit::registerRunningThread(mvm::Thread* th) {
	vmkitLock();
	numberOfRunningThreads++;
	th->remove();
	th->appendTo(&runningThreads);
	vmkitUnlock();
}
  
void VMKit::unregisterRunningThread(mvm::Thread* th) {
	vmkitLock();
	numberOfRunningThreads--;
	th->remove();
	th->appendTo(&preparedThreads);
	vmkitUnlock();
}
