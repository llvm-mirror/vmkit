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

