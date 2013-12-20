#include "vmkit/thread.h"

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
	framePointer = (void**)__builtin_frame_address(1);
	next(initialPop);
}

bool StackWalker::next(uint32_t nbPop) {
	while(nbPop--) {
		void** next = (void**)framePointer[0];
		if(!next || !next[0])
			return 0;
		framePointer = next;
	}
	return 1;
}
	
void* StackWalker::ip() {
	return framePointer[1];
}
