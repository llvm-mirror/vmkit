#include "j3/j3trampoline.h"
#include "j3/j3object.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3class.h"
#include "j3/j3.h"

using namespace j3;

void* J3Trampoline::interfaceTrampoline(J3Object* obj) {
	J3ObjectHandle* prev = J3Thread::get()->tell();
	J3ObjectHandle* handle = J3Thread::get()->push(obj);
	J3ObjectType* type = obj->vt()->type()->asObjectType();
	uint32_t index = J3Thread::get()->interfaceMethodIndex() % J3VirtualTable::nbInterfaceMethodTable;
	J3InterfaceSlotDescriptor* desc = type->slotDescriptorAt(index);
	void* res;

	if(desc->nbMethods == 1) {
		res = desc->methods[0]->fnPtr(0);
		handle->vt()->_interfaceMethodTable[index] = res;
	} else
		J3::internalError(L"implement me: interface Trampoline with collision");

	J3Thread::get()->restore(prev);

	return res;
}

void* J3Trampoline::staticTrampoline(J3Object* obj, J3Method* target) {
	return target->fnPtr(0);
}
	
void* J3Trampoline::virtualTrampoline(J3Object* obj, J3Method* target) {
	J3ObjectHandle* prev = J3Thread::get()->tell();
	J3ObjectHandle* handle = J3Thread::get()->push(obj);
	J3ObjectType* cl = handle->vt()->type()->asObjectType();
	J3Method* impl = cl == target->cl() ? target : cl->findVirtualMethod(target->name(), target->sign());

	void* res = impl->fnPtr(0);
	handle->vt()->virtualMethods()[impl->index()] = res;

	J3Thread::get()->restore(prev);

	return res;
}

void* J3Trampoline::buildTrampoline(vmkit::BumpAllocator* allocator, J3Method* m, void* tra) {	
	size_t trampolineSize = 148;
	void* res = allocator->allocate(trampolineSize);

#define dd(p, n) ((((uintptr_t)p) >> n) & 0xff)
	uint8_t code[] = {
		0x57, // 0: push %rdi
		0x56, // 1: push %rsi
		0x52, // 2: push %rdx
		0x51, // 3: push %rcx
		0x41, 0x50, // 4: push %r8
		0x41, 0x51, // 6: push %r9
		0x48, 0x81, 0xec, 0x88, 0x00, 0x00, 0x00, // 8: sub $128+8, %esp
		0xf3, 0x0f, 0x11, 0x04, 0x24,             // 15: movss %xmm0, (%rsp)
		0xf3, 0x0f, 0x11, 0x4c, 0x24, 0x10,       // 20: movss %xmm1, 16(%rsp)
		0xf3, 0x0f, 0x11, 0x54, 0x24, 0x20,       // 26: movss %xmm2, 32(%rsp)
		0xf3, 0x0f, 0x11, 0x5c, 0x24, 0x30,       // 32: movss %xmm3, 48(%rsp)
		0xf3, 0x0f, 0x11, 0x64, 0x24, 0x40,       // 38: movss %xmm4, 64(%rsp)
		0xf3, 0x0f, 0x11, 0x6c, 0x24, 0x50,       // 44: movss %xmm5, 80(%rsp)
		0xf3, 0x0f, 0x11, 0x74, 0x24, 0x60,       // 50: movss %xmm6, 96(%rsp)
		0xf3, 0x0f, 0x11, 0x7c, 0x24, 0x70,       // 56: movss %xmm7, 112(%rsp)
		0x48, 0xbe, dd(m, 0), dd(m, 8), dd(m, 16), dd(m, 24), dd(m, 32), dd(m, 40), dd(m, 48), dd(m, 56), // 62: mov -> %rsi
		0x48, 0xb8, dd(tra, 0), dd(tra, 8), dd(tra, 16), dd(tra, 24), dd(tra, 32), dd(tra, 40), dd(tra, 48), dd(tra, 56), // 72: mov -> %rax
		0xff, 0xd0, // 82: call %rax
		0xf3, 0x0f, 0x10, 0x04, 0x24,             // 84: movss (%rsp), %xmm0
		0xf3, 0x0f, 0x10, 0x4c, 0x24, 0x10,       // 89: movss 16(%rsp), %xmm1
		0xf3, 0x0f, 0x10, 0x54, 0x24, 0x20,       // 95: movss 32(%rsp), %xmm2
		0xf3, 0x0f, 0x10, 0x5c, 0x24, 0x30,       // 101: movss 48(%rsp), %xmm3
		0xf3, 0x0f, 0x10, 0x64, 0x24, 0x40,       // 107: movss 64(%rsp), %xmm4
		0xf3, 0x0f, 0x10, 0x6c, 0x24, 0x50,       // 113: movss 80(%rsp), %xmm5
		0xf3, 0x0f, 0x10, 0x74, 0x24, 0x60,       // 119: movss 96(%rsp), %xmm6
		0xf3, 0x0f, 0x10, 0x7c, 0x24, 0x70,       // 125: movss 112(%rsp), %xmm7
		0x48, 0x81, 0xc4, 0x88, 0x00, 0x00, 0x00, // 131: add $128+8, %esp
		0x41, 0x59, // 138: pop %r9
		0x41, 0x58, // 140: pop %r8
		0x59, // 142: pop %rcx
		0x5a, // 143: pop %rdx
		0x5e, // 144: pop %rsi
		0x5f, // 145: pop %rdi
		0xff, 0xe0 // 146: jmp %rax
		// total: 148
	};
#undef dd

	memcpy(res, code, trampolineSize);

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
