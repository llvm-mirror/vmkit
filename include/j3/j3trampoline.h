#ifndef _J3_TRAMPOLINE_H_
#define _J3_TRAMPOLINE_H_

#include <stdint.h>

extern "C" char       trampoline_generic;
extern "C" char       trampoline_generic_method;
extern "C" char       trampoline_generic_resolver; 
extern "C" char       trampoline_generic_end;
extern "C" void       trampoline_restart(void* ptr, void* saveZone);

namespace vmkit {
	class BumpAllocator;
}

namespace j3 {
	class J3Method;
	class J3Object;

	class J3Trampoline {
		static void  interfaceTrampoline(J3Object* obj);
		static void  staticTrampoline(J3Object* obj, J3Method* ref);
		static void  virtualTrampoline(J3Object* obj, J3Method* ref);

		static void* buildTrampoline(vmkit::BumpAllocator* allocator, J3Method* method, void* tra);
	public:
		static void  initialize(uintptr_t mask);

		static void* buildStaticTrampoline(vmkit::BumpAllocator* allocator, J3Method* target);
		static void* buildVirtualTrampoline(vmkit::BumpAllocator* allocator, J3Method* target);
		static void* buildInterfaceTrampoline(vmkit::BumpAllocator* allocator);
	};
};

#endif
