//===----------- Allocator.h - A memory allocator  ------------------------===//
//
//                        The VMKit project
//
//===----------------------------------------------------------------------===//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>

namespace vmkit {
	class BumpAllocatorNode { /* always the first bytes of a bucket */
	public:
		BumpAllocatorNode* next;
		uint8_t*                top;
	};

	class BumpAllocator {
		static const size_t bucketSize = 64*1024;
	private:
		pthread_mutex_t         mutex;
		BumpAllocatorNode* current;

		void* operator new(size_t n);
		BumpAllocator(); /* private to force an allocation via the new operator */
		~BumpAllocator();
		void operator delete(void* p);
	public:
		static size_t round(uint64_t n, uint64_t r) { return ((n - 1) & -r) + r; }

		static BumpAllocator* create();
		static void           destroy(BumpAllocator* allocator);

		static void* map(size_t n);
		static void  unmap(void* p, size_t n); 

		void* allocate(size_t size);

	};

	class PermanentObject {
		void* operator new(size_t size) { return 0; }
	public:
		void* operator new(size_t size, BumpAllocator* allocator) {
			return allocator->allocate(size);
		}

		void* operator new [](size_t size, BumpAllocator* allocator) {
			return allocator->allocate(size);
		}
  
		void operator delete(void* ptr);
  
		void operator delete[](void* ptr);
	};

	template <typename T>
	class StdAllocator : public PermanentObject {
	public:
    typedef T          value_type;
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
 
    typedef T*         pointer;
    typedef const T*   const_pointer;
 
    typedef T&         reference;
    typedef const T&   const_reference;

		template <class U> struct rebind { typedef StdAllocator<U> other; };

		size_type max_size( ) const { return 0x1000L*0x1000L; }

		BumpAllocator* _allocator;

    template <class U>
		StdAllocator(const StdAllocator<U> &other) {
			_allocator = other._allocator;
		}

		StdAllocator(BumpAllocator* allocator) {
			_allocator = allocator;
		}

		pointer allocate(size_type n, const void* hint=0) {
			pointer res = (pointer)_allocator->allocate(n*sizeof(T));
			//printf("allocate %lu object at %p\n", n, res);
			return res;
		}

		void deallocate(pointer p, size_type n) {
			PermanentObject::operator delete(p);
    }

		pointer       address(reference r) const { return &r; }
    const_pointer address(const_reference r) const { return &r; }

		void construct(pointer p, const T& val) {
			new (p) T(val);
    }

		void destroy(pointer p) {
			p->~T();
    }
	};

	template<typename T>
	struct T_ptr_less_t : public PermanentObject {
    bool operator()(const T& lhs, const T& rhs) const { return lhs < rhs; } 
	};
} // end namespace vmkit

#endif // VMKIT_ALLOCATOR_H
