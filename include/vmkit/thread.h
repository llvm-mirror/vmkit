#ifndef _THREAD_H_
#define _THREAD_H_

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <signal.h>

#include "vmkit/allocator.h"

namespace vmkit {
	class VMKit;

	class Thread {
	public:
		typedef void (*sa_action_t)(int, siginfo_t *, void *);

	private:
		VMKit*               _vm;
		pthread_t            _tid;

		static void* doRun(void* thread);
		static void sigsegvHandler(int n, siginfo_t* info, void* context);

		void registerSignalInternal(int n, sa_action_t handler, bool altStack);
	public:
		Thread(VMKit* vm);
		virtual ~Thread() {}

		void* operator new(size_t n);
		void operator delete(void* p);

		virtual void run() {}

		VMKit* vm() { return _vm; }

		static uintptr_t getThreadMask();
		static Thread* get(void* ptr);
		static Thread* get();

		bool registerSignal(int n, sa_action_t handler);

		void start();
		void join();
	};

	class StackWalker {
		unw_cursor_t  cursor; 
		unw_context_t uc;

	public:
		StackWalker(uint32_t initialPop=0) __attribute__((noinline));

		bool  next(uint32_t nbPop=1);
		void* ip();
		void* sp();
	};
}

#endif
