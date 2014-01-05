#include "vmkit/thread.h"
#include "vmkit/system.h"
#include "vmkit/vmkit.h"

using namespace vmkit;

Thread::Thread(VMKit* vm) { 
	_vm = vm; 
}

void Thread::sigsegvHandler(int n, siginfo_t* info, void* context) {
	get()->vm()->sigsegv((uintptr_t)info->si_addr);
}

void* Thread::operator new(size_t n) {
	return ThreadAllocator::allocator()->allocate();
}

void Thread::operator delete(void* p) {
	ThreadAllocator::allocator()->release(p);
}

Thread* Thread::get() {
	return get(__builtin_frame_address(0));
}

Thread* Thread::get(void* ptr) {
	return (Thread*)((uintptr_t)ptr & getThreadMask());
}

uintptr_t Thread::getThreadMask() {
	return ThreadAllocator::allocator()->magic();
}

void Thread::registerSignalInternal(int n, sa_action_t handler, bool altStack) {
  // Set the SIGSEGV handler to diagnose errors.
  struct sigaction sa;
  sigset_t mask;
  sigfillset(&mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
	if(altStack)
		sa.sa_flags = SA_ONSTACK;
  sa.sa_mask = mask;
  sa.sa_sigaction = handler;
  sigaction(n, &sa, NULL);
}

bool Thread::registerSignal(int n, sa_action_t handler) {
	if(n == SIGSEGV || n == SIGBUS)
		return 0;
	registerSignalInternal(n, handler, 0);
	return 1;
}

void* Thread::doRun(void* _thread) {
	Thread* thread = (Thread*)_thread;

  // Set the alternate stack as the second page of the thread's
  // stack.
  stack_t st;
  st.ss_sp = ThreadAllocator::allocator()->alternateStackAddr(thread);
  st.ss_flags = 0;
  st.ss_size = ThreadAllocator::allocator()->alternateStackSize(thread);
  sigaltstack(&st, NULL);

	thread->registerSignalInternal(SIGSEGV, sigsegvHandler, 1);
	thread->registerSignalInternal(SIGBUS, sigsegvHandler, 1);

	thread->run();
	return 0;
}

void Thread::start() {
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, ThreadAllocator::allocator()->stackAddr(this), ThreadAllocator::allocator()->stackSize(this));

	pthread_create(&_tid, &attr, doRun, this);

	pthread_attr_destroy(&attr);
}

void Thread::join() {
	void* res;
	pthread_join(_tid, &res);
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
