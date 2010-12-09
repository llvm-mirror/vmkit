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

class LauncherThread : public Thread {
public:
 	void (*realStart)(VirtualMachine*, int, char**);
 	VirtualMachine *vm;
 	int argc;
 	char** argv;

 	LauncherThread(VMKit* vmkit, void (*s)(VirtualMachine*, int, char**), VirtualMachine* v, int ac, char** av) : Thread(vmkit) {
 		realStart = s;
 		vm = v;
 		argc = ac;
 		argv = av;
 	}

 	static void launch(LauncherThread* th) {
 		if(th->realStart)
 			th->realStart(th->vm, th->argc, th->argv);
 		else
 			th->vm->runApplicationImpl(th->argc, th->argv);
 	}
};

void VirtualMachine::runApplication0(void (*starter)(VirtualMachine*, int, char**), int argc, char **argv) {
	(new LauncherThread(vmkit, starter, this, argc, argv))->start((void (*)(Thread*))LauncherThread::launch);
}

void VirtualMachine::runApplication0(int argc, char** argv) {
	runApplication0(0, argc, argv);
}
