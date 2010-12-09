#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#define NI() fprintf(stderr, "Not implemented\n"); abort()

namespace toyvm {

	class ToyVM : public mvm::VirtualMachine {
	public:
		ToyVM(mvm::BumpPtrAllocator &alloc, mvm::VMKit* vmkit) : mvm::VirtualMachine(alloc, vmkit) {}

		virtual mvm::gc** getReferent(mvm::gc*) { NI(); }
		virtual void setReferent(mvm::gc*, mvm::gc*) { NI(); }
		virtual bool enqueueReference(mvm::gc*) { NI(); }
		virtual size_t getObjectSize(mvm::gc*) { NI(); }
		virtual void runApplication(int, char**) { NI(); }
	};

	class ToyVMThread : public mvm::VMThreadData {
	};

}

using namespace toyvm;

int main(int argc, char **argv) {
  // Initialize base components.  
  mvm::BumpPtrAllocator Allocator;
	mvm::VMKit* vmkit = new(Allocator, "VMKit") mvm::VMKit(Allocator);

	new(Allocator, "toy vm") ToyVM(Allocator, vmkit);

	return 0;
}
