#ifndef _J3_TRAMPOLINE_H_
#define _J3_TRAMPOLINE_H_

namespace vmkit {
	class BumpAllocator;
}

namespace j3 {
	class J3Method;
	class J3Object;

	class J3Trampoline {
		static void* staticTrampoline(J3Object* obj, J3Method* ref);
		static void* virtualTrampoline(J3Object* obj, J3Method* ref);

		static void* buildTrampoline(vmkit::BumpAllocator* allocator, J3Method* method, void* tra);
	public:
		static void* buildStaticTrampoline(vmkit::BumpAllocator* allocator, J3Method* target);
		static void* buildVirtualTrampoline(vmkit::BumpAllocator* allocator, J3Method* target);
	};
};

#endif
