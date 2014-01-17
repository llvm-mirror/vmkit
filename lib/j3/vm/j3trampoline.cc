#include "j3/j3trampoline.h"
#include "j3/j3object.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3class.h"
#include "j3/j3.h"

using namespace j3;

uintptr_t J3Trampoline::argOffset = (uint64_t)&((J3Thread*)0)->_trampolineArg;

void J3Trampoline::interfaceTrampoline(J3Object* obj) {
	J3TrampolineArg arg = J3Thread::get()->_trampolineArg;

	J3ObjectHandle* prev = J3Thread::get()->tell();
	J3ObjectHandle* handle = J3Thread::get()->push(obj);
	J3ObjectType* type = obj->vt()->type()->asObjectType();
	uint32_t index = J3Thread::get()->interfaceMethodIndex() % J3VirtualTable::nbInterfaceMethodTable;
	J3InterfaceSlotDescriptor* desc = type->slotDescriptorAt(index);
	void* res;

	if(desc->nbMethods == 1) {
		desc->methods[0]->ensureCompiled(0);
		res = desc->methods[0]->fnPtr();
		handle->vt()->_interfaceMethodTable[index] = res;
	} else {
		for(uint32_t i=0; i<desc->nbMethods; i++)
			fprintf(stderr, "   method: %s::%s%s - %d\n", desc->methods[i]->cl()->name()->cStr(),
							desc->methods[i]->name()->cStr(), desc->methods[i]->signature()->name()->cStr(),
							desc->methods[i]->interfaceIndex());
		J3::internalError("implement me: interface Trampoline with collision: %d", desc->nbMethods);
	}

	J3Thread::get()->restore(prev);

	trampoline_restart(res, &arg);
}

void J3Trampoline::staticTrampoline(J3Object* obj, J3Method* target) {
	J3TrampolineArg arg = J3Thread::get()->_trampolineArg;

	target->ensureCompiled(0);

	trampoline_restart(target->fnPtr(), &arg);
}
	
void J3Trampoline::virtualTrampoline(J3Object* obj, J3Method* target) {
	J3TrampolineArg arg = J3Thread::get()->_trampolineArg;
	J3ObjectHandle* prev = J3Thread::get()->tell();
	J3ObjectHandle* handle = J3Thread::get()->push(obj);
	J3ObjectType* cl = handle->vt()->type()->asObjectType();
	J3Method* impl = cl == target->cl() ? target : cl->findMethod(0, target->name(), target->signature());

	impl->ensureCompiled(0);
	void* res = impl->fnPtr();
	handle->vt()->virtualMethods()[impl->index()] = res;

	J3Thread::get()->restore(prev);

	trampoline_restart(res, &arg);
}

void* J3Trampoline::buildTrampoline(vmkit::BumpAllocator* allocator, J3Method* m, void* tra) {	
	size_t trampolineSize = &trampoline_generic_end - &trampoline_generic;
	char* res = (char*)allocator->allocate(trampolineSize);

	memcpy(res, &trampoline_generic, trampolineSize);

	*((void**)(res + (&trampoline_generic_method - &trampoline_generic))) = (void*)m;
	*((void**)(res + (&trampoline_generic_resolver - &trampoline_generic))) = tra;

	return res;
}

void* J3Trampoline::buildStaticTrampoline(vmkit::BumpAllocator* allocator, J3Method* method) {
	return buildTrampoline(allocator, method, (void*)staticTrampoline);
}

void* J3Trampoline::buildVirtualTrampoline(vmkit::BumpAllocator* allocator, J3Method* method) {
	return buildTrampoline(allocator, method, (void*)virtualTrampoline);
}

void* J3Trampoline::buildInterfaceTrampoline(vmkit::BumpAllocator* allocator) {
	return buildTrampoline(allocator, 0, (void*)interfaceTrampoline);
}

