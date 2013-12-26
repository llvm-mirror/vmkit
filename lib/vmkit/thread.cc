#include "vmkit/thread.h"
#include "vmkit/system.h"

using namespace vmkit;

__thread Thread* Thread::_thread = 0;

Thread::Thread(VMKit* vm, BumpAllocator* allocator) { 
	_allocator = allocator;
	_vm = vm; 
	_baseFramePointer = 0;
}

void Thread::destroy(Thread* thread) {
	BumpAllocator::destroy(thread->allocator());
}

void* Thread::doRun(void* _thread) {
	Thread* thread = (Thread*)_thread;
	set(thread);
	thread->_entryPoint();
	return 0;
}

void Thread::start(entryPoint_t entryPoint, Thread* thread) {
	thread->_entryPoint = entryPoint;
	pthread_t tid;
	pthread_create(&tid, 0, doRun, thread);
}

StackWalker::StackWalker(uint32_t initialPop) {
	framePointer = System::current_fp();
	next(initialPop+1);
}

bool StackWalker::next(uint32_t nbPop) {
	while(nbPop--) {
		if(framePointer == Thread::get()->baseFramePointer())
			return 0;
		framePointer = (void**)System::fp_to_next_fp(framePointer);
	}
	return framePointer != Thread::get()->baseFramePointer();
}
	
void* StackWalker::ip() {
	return System::fp_to_ip(framePointer);
}
