#include "j3/j3thread.h"
#include "j3/j3.h"

using namespace j3;

J3Thread::J3Thread(J3* vm, vmkit::BumpAllocator* allocator) : 
	Thread(vm, allocator),
	_fixedPoint(allocator) {
	_jniEnv.functions = &jniEnvTable;
}

J3Thread* J3Thread::create(J3* j3) {
	vmkit::BumpAllocator* allocator = vmkit::BumpAllocator::create();
	return new(allocator) J3Thread(j3, allocator);
}

void J3Thread::ensureCapacity(uint32_t capacity) {
	_fixedPoint.unsyncEnsureCapacity(capacity);
}

J3ObjectHandle* J3Thread::push(J3ObjectHandle* handle) { 
	return _fixedPoint.unsyncPush(handle); 
}

J3ObjectHandle* J3Thread::push(J3Object* obj) { 
	return _fixedPoint.unsyncPush(obj); 
}

J3ObjectHandle* J3Thread::tell() { 
	return _fixedPoint.unsyncTell(); 
}

void J3Thread::restore(J3ObjectHandle* obj) { 
	_fixedPoint.unsyncRestore(obj); 
}

J3Thread* J3Thread::get() { 
	return (J3Thread*)Thread::get(); 
}
