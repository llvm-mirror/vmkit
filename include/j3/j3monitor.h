#ifndef _J3_MONITOR_H_
#define _J3_MONITOR_H_

#include <pthread.h>
#include <stdint.h>

namespace vmkit {
	class BumpAllocator;
}

namespace j3 {
	class J3Thread;
	class J3Object;

	class J3LockRecord {
	public:
		static const uint32_t gepHeader = 0;
		static const uint32_t gepLockCount = 1;

		uintptr_t header;
		uint32_t  lockCount;
	};

	class J3Monitor {
		friend class J3MonitorManager;
		friend class J3Object;

		J3Monitor*      _next;

		J3Thread*       owner;
		uint32_t        lockCount;
		pthread_mutex_t mutex;
		pthread_cond_t  cond;
		uintptr_t       header;
		J3Object*       object;
		J3LockRecord*   record;

		void checkRecord();

		void init(J3Monitor* next); /* acquire the lock for the next inflate */		
	public:
		bool isDeflatable(); /* acquire the lock for the next inflate */
		void prepare(J3Object* _object, uintptr_t header, J3LockRecord* _record);

		void lock();
		void unlock();

		void wait();
		void timed_wait(uint64_t ms, uint32_t ns);

		void notify();
		void notifyAll();
	};

	class J3MonitorManager {
		static const uint32_t monitorsBucket = 256;
		pthread_mutex_t       mutex;
		vmkit::BumpAllocator* allocator;
		J3Monitor*            head;

	public:
		J3MonitorManager(vmkit::BumpAllocator* _allocator);

		J3Monitor* allocate();
		void       release(J3Monitor* monitor); 
	};

}

#endif
