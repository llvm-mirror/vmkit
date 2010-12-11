#include "mvm/VMKit.h"
#include "../../lib/p3/VMCore/P3.h"

using namespace p3;

int main(int argc, char **argv) {
  // Initialize base components.  
  mvm::BumpPtrAllocator Allocator;
	mvm::VMKit* vmkit = new(Allocator, "VMKit") mvm::VMKit(Allocator);
 
  // Create the allocator that will allocate the bootstrap loader and the JVM.
	p3::P3* vm = new(Allocator, "VM") P3(Allocator, vmkit);
 
  // Run the application. 
  vm->runApplication(argc, argv);
  vmkit->waitNonDaemonThreads();
  exit(0);
}

