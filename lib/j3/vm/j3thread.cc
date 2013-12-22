#include "j3/j3thread.h"
#include "j3/j3method.h"
#include "j3/j3.h"

#include "vmkit/safepoint.h"
#include "vmkit/compiler.h"

using namespace j3;

J3Thread::J3Thread(J3* vm, vmkit::BumpAllocator* allocator) : 
	Thread(vm, allocator),
	_localReferences(allocator) {
	_jniEnv.functions = &jniEnvTable;
}

J3Thread* J3Thread::create(J3* j3) {
	vmkit::BumpAllocator* allocator = vmkit::BumpAllocator::create();
	return new(allocator) J3Thread(j3, allocator);
}

J3Method* J3Thread::getJavaCaller(uint32_t level) {
	vmkit::Safepoint* sf = 0;
	vmkit::StackWalker walker;

	while(walker.next()) {
		vmkit::Safepoint* sf = vm()->getSafepoint(walker.ip());

		if(sf && !level--)
			return ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
	}

	return 0;
}

J3ObjectHandle* J3Thread::pendingException() {
	if(_pendingException) {
		return push(_pendingException);
	} else
		return 0;
}

void J3Thread::ensureCapacity(uint32_t capacity) {
	_localReferences.ensureCapacity(capacity);
}

J3ObjectHandle* J3Thread::push(J3ObjectHandle* handle) { 
	return _localReferences.push(handle); 
}

J3ObjectHandle* J3Thread::push(J3Object* obj) { 
	return _localReferences.push(obj); 
}

J3ObjectHandle* J3Thread::tell() { 
	return _localReferences.tell(); 
}

void J3Thread::restore(J3ObjectHandle* ptr) { 
	_localReferences.restore(ptr); 
}

J3Thread* J3Thread::get() { 
	return (J3Thread*)Thread::get(); 
}
