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

		pthread_mutex_t mutex;
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
			pthread_mutex_init(&mutex, 0);
			allocator = _allocator;
			head = 0;
			createNode();
		}

		void unsyncEnsureCapacity(uint32_t capacity) {
			T* reserve = head->top + capacity;
			if(reserve > head->max)
				createNode(capacity);
		}

		T* syncPush() { 
			StackNode<T>* cur = head;
			T* res = (T*)__sync_fetch_and_add((uintptr_t*)&cur->top, (uintptr_t)sizeof(T));

			if(res < cur->max)
				return res;

			pthread_mutex_lock(&mutex);
			if(cur->nextFree)
				head = cur->nextFree;
			else
				createNode();
			pthread_mutex_unlock(&mutex);
			return syncPush();
		}

		T* unsyncPush() {
			T* res = head->top++;

			if(res < head->max)
				return res;

			if(head->nextFree)
				head = head->nextFree;
			else
				createNode();
			return unsyncPush();
		}

		void unsyncPop() {
			T* res = head->top - 1;
			if(res < (T*)(head + 1)) {
				head = head->nextBusy;
				head->top = (T*)(head+1);		
			} else
				head->top = res;
		}

		T* unsyncTell() { 
			return head->top; 
		}

		void unsyncRestore(T* ptr) {
			while(ptr <= (T*)head || ptr > head->max)
				head = head->nextBusy;
			head->top = ptr;
		}
	};
}

#endif
