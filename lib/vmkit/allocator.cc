#include "vmkit/allocator.h"
#include "vmkit/thread.h"
#include "vmkit/vmkit.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

using namespace vmkit;

void* BumpAllocator::operator new(size_t n) {
	return (void*)((uintptr_t)map(bucketSize) + sizeof(BumpAllocatorNode));
}

BumpAllocator::BumpAllocator() {
	pthread_mutex_init(&mutex, 0);
	current = (BumpAllocatorNode*)((uintptr_t)this - sizeof(BumpAllocatorNode));
	current->next = 0;
	current->top  = (uint8_t*)round((uintptr_t)(this + 1), 64);
}

BumpAllocator* BumpAllocator::create() {
	return new BumpAllocator();
}

BumpAllocator::~BumpAllocator() {
	while(current->next) {
		BumpAllocatorNode* tmp = current->next;
		unmap(current, (uintptr_t)current->top - (uintptr_t)current);
		current = tmp;
	}
}

void BumpAllocator::operator delete(void* p) {
	unmap(p, bucketSize);
}

void BumpAllocator::destroy(BumpAllocator* allocator) {
	delete allocator;
}

void* BumpAllocator::allocate(size_t size) {
	if(size > (bucketSize - sizeof(BumpAllocatorNode))) {
		pthread_mutex_lock(&mutex);
		size_t total = round((uintptr_t)size + sizeof(BumpAllocatorNode), 4096);
		BumpAllocatorNode* newBucket = (BumpAllocatorNode*)map(total);
		//memset(newBucket, 0, total);
		newBucket->next = current->next;
		current->next = newBucket;
		newBucket->top  = (uint8_t*)((uintptr_t)newBucket + bucketSize);
		pthread_mutex_unlock(&mutex);
		return newBucket + 1;
	}

	while(1) {
		BumpAllocatorNode* node = current;
		uint8_t* res = __sync_fetch_and_add(&node->top, (uint8_t*)round(size, 8));
		uint8_t* end = res + size;

		if(res >= (uint8_t*)node && (res + size) < ((uint8_t*)node) + bucketSize) {
			//printf("%p -> %lu %lu (%p -> %p)\n", res, size, round((uintptr_t)size, 64), node, ((uint8_t*)node) + bucketSize);
			//memset(res, 0, size);
			return res;
		}

		pthread_mutex_lock(&mutex);
		BumpAllocatorNode* newBucket = (BumpAllocatorNode*)map(bucketSize);
		newBucket->next = current;
		newBucket->top  = (uint8_t*)(newBucket + 1);
		current = newBucket;
		pthread_mutex_unlock(&mutex);
	}
}

void* BumpAllocator::map(size_t n) {
	void* res = mmap(0, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	if(res == MAP_FAILED)
		Thread::get()->vm()->internalError(L"unable to map %ld bytes", n);
	//memset(res, 0, n);
	return res;
}

void BumpAllocator::unmap(void* p, size_t n) {
	munmap(0, n);
}

void PermanentObject::operator delete(void* ptr) {
	Thread::get()->vm()->internalError(L"should not happen");
}
  
void PermanentObject::operator delete[](void* ptr) {
	Thread::get()->vm()->internalError(L"should not happen");
}
