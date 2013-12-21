#include "vmkit/thread.h"
#include "vmkit/system.h"

using namespace vmkit;

__thread Thread* Thread::_thread = 0;

Thread::Thread(VMKit* vm, BumpAllocator* allocator) { 
	_allocator = allocator;
	_vm = vm; 
}

void Thread::destroy(Thread* thread) {
	BumpAllocator::destroy(thread->allocator());
}

StackWalker::StackWalker(uint32_t initialPop) {
	framePointer = System::current_fp();
	next(initialPop+1);
}

bool StackWalker::next(uint32_t nbPop) {
	while(nbPop--) {
		void** next = (void**)System::fp_to_next_fp(framePointer);
		if(!next || !System::fp_to_next_fp(next))
			return 0;
		framePointer = next;
	}
	return 1;
}
	
void* StackWalker::ip() {
	return System::fp_to_ip(framePointer);
}
