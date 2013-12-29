#include "j3/j3monitor.h"
#include "j3/j3thread.h"
#include "j3/j3.h"

#include <sys/time.h>

using namespace j3;

void J3Monitor::init(J3Monitor* next) {
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, 0);
	pthread_mutex_lock(&mutex);
}

bool J3Monitor::isDeflatable() {
	return pthread_mutex_trylock(&mutex) == 0;
}

void J3Monitor::prepare(J3Object* _object, J3Thread* _owner, uint32_t _recursiveCount, uintptr_t _header) {
	object = _object;
	owner = _owner;
	recursiveCount = _recursiveCount;
	header = _header;
}

void J3Monitor::lock() {
	J3Thread* self = J3Thread::get();
	if(owner == self) {
		recursiveCount++;
	} else {
		pthread_mutex_lock(&mutex);
		recursiveCount++;
		owner = self;
	}
}

void J3Monitor::unlock() {
	J3Thread* self = J3Thread::get();
	if(owner != self)
		J3::illegalMonitorStateException();
	if(!--recursiveCount) {
		owner = 0;
		pthread_mutex_unlock(&mutex);
	} else
		__sync_synchronize(); /* JMM */
}
		
void J3Monitor::wait() {
	timed_wait(0, 0);
}

void J3Monitor::timed_wait(uint64_t ms, uint32_t ns) {
	J3Thread* self = J3Thread::get();

	if(owner != self)
		J3::illegalMonitorStateException();

	uint32_t r = recursiveCount;
	owner = 0;

	if(ms || ns) {
		struct timeval tv;
		struct timespec ts;
		gettimeofday(&tv, 0);
		ts.tv_sec = tv.tv_sec + ms / 1000;
		ts.tv_nsec = tv.tv_usec*1000 + ms % 1000 + ns;
		if(ts.tv_nsec > 1e9) {
			ts.tv_sec++;
			ts.tv_nsec -= 1e9;
		}
		pthread_cond_timedwait(&cond, &mutex, &ts);
	} else
		pthread_cond_wait(&cond, &mutex);

	owner = self;
	recursiveCount = r;
}

void J3Monitor::notify() {
	if(owner != J3Thread::get())
		J3::illegalMonitorStateException();
	pthread_cond_signal(&cond);
}

void J3Monitor::notifyAll() {
	if(owner != J3Thread::get())
		J3::illegalMonitorStateException();
	pthread_cond_broadcast(&cond);
}


J3MonitorManager::J3MonitorManager(vmkit::BumpAllocator* _allocator) {
	pthread_mutex_init(&mutex, 0);
	allocator = _allocator;
}

J3Monitor* J3MonitorManager::allocate() {
	pthread_mutex_lock(&mutex);

	J3Monitor* res = head;

	if(!res) {
		uint32_t n = monitorsBucket;
		J3Monitor* bucket = (J3Monitor*)allocator->allocate(sizeof(J3Monitor)*n);

		for(uint32_t i=0; i<n; i++)
			bucket[i].init(bucket+i-1);

		bucket[0]._next = head;
		head = bucket + n - 1;
		res = head;
	}
	head = res->_next;
	pthread_mutex_unlock(&mutex);
	return res;
}

void J3MonitorManager::release(J3Monitor* monitor) {
	pthread_mutex_lock(&mutex);
	monitor->_next = head;
	head = monitor;
	pthread_mutex_unlock(&mutex);
}

