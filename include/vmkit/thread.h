#ifndef _THREAD_H_
#define _THREAD_H_

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "vmkit/allocator.h"

namespace vmkit {
	class VMKit;

	class Thread {
		VMKit*               _vm;
		pthread_t            _tid;

		static void* doRun(void* thread);

	public:
		Thread(VMKit* vm);
		virtual ~Thread() {}

		void* operator new(size_t n);
		void operator delete(void* p);

		virtual void run() {}

		VMKit* vm() { return _vm; }

		static Thread* get();

		void start();
		void join();

		static uintptr_t getThreadMask();
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
