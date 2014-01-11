#include "j3/j3trampoline.h"
#include "j3/j3object.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3class.h"
#include "j3/j3.h"


using namespace j3;

void* J3Trampoline::interfaceTrampoline(J3Object* obj) {
	J3::internalError("implement me it");
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
			fprintf(stderr, "   method: %s::%s%s\n", desc->methods[i]->cl()->name()->cStr(),
							desc->methods[i]->name()->cStr(), desc->methods[i]->signature()->name()->cStr());
		J3::internalError("implement me: interface Trampoline with collision: %d", desc->nbMethods);
	}

	J3Thread::get()->restore(prev);

	return res;
}

void* J3Trampoline::staticTrampoline(J3Object* obj, J3Method* target) {
	char saveZone[TRAMPOLINE_SAVE_ZONE];
	memcpy(saveZone, J3Thread::get()->_trampolineSaveZone, TRAMPOLINE_SAVE_ZONE);
	target->ensureCompiled(0);
	//return target->fnPtr();
	trampoline_restart(target->fnPtr(), saveZone);
	return 0;
}
	
void* J3Trampoline::virtualTrampoline(J3Object* obj, J3Method* target) {
	J3::internalError("implement me vt");
	J3ObjectHandle* prev = J3Thread::get()->tell();
	J3ObjectHandle* handle = J3Thread::get()->push(obj);
	J3ObjectType* cl = handle->vt()->type()->asObjectType();
	J3Method* impl = cl == target->cl() ? target : cl->findMethod(0, target->name(), target->signature());

	impl->ensureCompiled(0);
	void* res = impl->fnPtr();
	handle->vt()->virtualMethods()[impl->index()] = res;

	J3Thread::get()->restore(prev);

	return res;
}

void* J3Trampoline::buildTrampoline(vmkit::BumpAllocator* allocator, J3Method* m, void* tra) {	
	size_t trampolineSize = &trampoline_generic_end - &trampoline_generic;
	char* res = (char*)allocator->allocate(trampolineSize);

	memcpy(res, &trampoline_generic, trampolineSize);

	*((char**)(res + (&trampoline_generic_save - &trampoline_generic))) = &trampoline_save;
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

void J3Trampoline::initialize(uintptr_t mask) {
	J3Thread* thread = J3Thread::get();
	trampoline_mask = mask;
	trampoline_offset = (uintptr_t)&thread->_trampolineSaveZone - (uintptr_t)thread;
}

