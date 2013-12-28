#include "vmkit/thread.h"
#include "vmkit/system.h"
#include "vmkit/vmkit.h"

using namespace vmkit;

__thread Thread* Thread::_thread = 0;

Thread::Thread(VMKit* vm, BumpAllocator* allocator) { 
	_allocator = allocator;
	_vm = vm; 
}

void Thread::destroy(Thread* thread) {
	BumpAllocator::destroy(thread->allocator());
}

void* Thread::doRun(void* _thread) {
	Thread* thread = (Thread*)_thread;
	set(thread);
	thread->run();
	return 0;
}

void Thread::start() {
	pthread_t tid;
	pthread_create(&tid, 0, doRun, this);
}

StackWalker::StackWalker(uint32_t initialPop) {
	unw_getcontext(&uc);
  unw_init_local(&cursor, &uc);
	next(initialPop+1);
}

bool StackWalker::next(uint32_t nbPop) {
	while(nbPop--) {
		if(unw_step(&cursor) <= 0)
			return 0;
	}
	return 1;
}
	
void* StackWalker::ip() {
	unw_word_t ip;
	unw_get_reg(&cursor, UNW_REG_IP, &ip);
	return (void*)ip;
}

void* StackWalker::sp() {
	unw_word_t sp;
	unw_get_reg(&cursor, UNW_REG_SP, &sp);
	return (void*)sp;
}
