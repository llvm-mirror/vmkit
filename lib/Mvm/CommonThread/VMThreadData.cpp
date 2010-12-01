#include "mvm/Threads/Thread.h"
#include "MutatorThread.h"

using namespace mvm;

void VMThreadData::attach() {
	mut->vmData = this;
	mut->MyVM = vm;
}
