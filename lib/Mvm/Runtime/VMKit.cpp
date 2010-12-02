#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"

using namespace mvm;

size_t VMKit::addVM(VirtualMachine* vm) {
	lockvms.lock();
	for(size_t i=0; i<vms.size(); i++)
		if(!vms[i]) {
			vms[i] = vm;
			return i;
		}
	int res = vms.size();
	vms.push_back(vm);
	lockvms.unlock();
	return res;
}

void VMKit::removeVM(size_t id) {
	vms[id] = 0;
}
