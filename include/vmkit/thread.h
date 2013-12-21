#ifndef _THREAD_H_
#define _THREAD_H_

#include "vmkit/allocator.h"

namespace vmkit {
	class VMKit;

	class Thread : protected PermanentObject {
		BumpAllocator*       _allocator;
		VMKit*               _vm;
		void*                _baseFramePointer;

	protected:
		Thread(VMKit* vm, BumpAllocator* allocator);

	public:
		static void destroy(Thread* thread);

		void  setBaseFramePointer(void* fp) { _baseFramePointer = fp; }
		void* baseFramePointer() { return _baseFramePointer; }

		VMKit* vm() { return _vm; }
		BumpAllocator* allocator() { return _allocator; }

		static __thread Thread* _thread;

		static Thread* get()          { return _thread; }
		static void set(Thread* thread) { _thread = thread; }
	};

	class StackWalker {
		void** framePointer;

	public:
		StackWalker(uint32_t initialPop=0) __attribute__((noinline));

		bool  next(uint32_t nbPop=1);
		void* ip();
		void* fp() { return framePointer; }
	};
}

#endif
