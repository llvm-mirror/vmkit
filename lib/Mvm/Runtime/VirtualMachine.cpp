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

void VirtualMachine::leaveNonDeamonMode() {
  nonDaemonLock.lock();
  --nbNonDaemonThreads;
  if (nbNonDaemonThreads == 0) nonDaemonVar.signal();
  nonDaemonLock.unlock();  
}

void VirtualMachine::enterNonDeamonMode() {
  nonDaemonLock.lock();
  ++nbNonDaemonThreads;
  nonDaemonLock.unlock();  
}

void VirtualMachine::waitForNonDeamonThreads() { 
	nonDaemonLock.lock();
  
  while (nbNonDaemonThreads) {
    nonDaemonVar.wait(&nonDaemonLock);
  }
  
  nonDaemonLock.unlock();
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
		th->vm->leaveNonDeamonMode();
	}
};

void VirtualMachine::runInNonDeamonThread(void (*start)(VirtualMachine*, int, char**), int argc, char **argv) {
  enterNonDeamonMode();
  (new LauncherThread(vmkit, start, this, argc, argv))->start((void (*)(Thread*))LauncherThread::launch);
}
