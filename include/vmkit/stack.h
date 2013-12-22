#ifndef _STACK_H_
#define _STACK_H_

#include "vmkit/allocator.h"

namespace vmkit {
	template <class T>
	class StackNode {
	public:
		StackNode<T>* nextFree;
		StackNode<T>* nextBusy;
		T*            top;
		T*            max;
	};

	template <class T>
	class Stack {
		static const uint32_t defaultNodeCapacity = 256;

		BumpAllocator*  allocator;
		StackNode<T>*   head;

		void createNode(uint32_t capacity=defaultNodeCapacity) {
			uint64_t size = capacity * sizeof(T) + sizeof(StackNode<T>);
			StackNode<T>* nn = (StackNode<T>*)allocator->allocate(size);
			nn->top = (T*)(nn + 1);
			nn->max = (T*)((uintptr_t)nn + size);
			nn->nextFree = 0;
			nn->nextBusy = head;
			if(head)
				head->nextFree = nn;
			head = nn;
		}

	public:
		Stack(BumpAllocator* _allocator) {
			allocator = _allocator;
			head = 0;
			createNode();
		}

		void ensureCapacity(uint32_t capacity) {
			T* reserve = head->top + capacity;
			if(reserve > head->max)
				createNode(capacity);
		}

		T* push() {
			T* res = head->top++;

			if(res < head->max)
				return res;

			if(head->nextFree)
				head = head->nextFree;
			else
				createNode();
			return push();
		}

		void pop() {
			T* res = head->top - 1;
			if(res < (T*)(head + 1)) {
				head = head->nextBusy;
				head->top = (T*)(head+1);		
			} else
				head->top = res;
		}

		T* tell() { 
			return head->top; 
		}

		void restore(T* ptr) {
			while(ptr <= (T*)head || ptr > head->max)
				head = head->nextBusy;
			head->top = ptr;
		}
	};
}

#endif
