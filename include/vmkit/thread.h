#ifndef _THREAD_H_
#define _THREAD_H_

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "vmkit/allocator.h"

namespace vmkit {
	class VMKit;

	class Thread : protected PermanentObject {
		typedef void (*entryPoint_t)();

		BumpAllocator*       _allocator;
		VMKit*               _vm;
		entryPoint_t         _entryPoint;

		static void* doRun(void* thread);

	protected:
		Thread(VMKit* vm, BumpAllocator* allocator);

	public:
		static void destroy(Thread* thread);

		VMKit* vm() { return _vm; }
		BumpAllocator* allocator() { return _allocator; }

		static __thread Thread* _thread;

		static Thread* get()          { return _thread; }
		static void set(Thread* thread) { _thread = thread; }

		static void start(entryPoint_t entryPoint, Thread* thread);
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
