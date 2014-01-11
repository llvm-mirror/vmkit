#ifndef _J3_ARCH_DEP_H_
#define _J3_ARCH_DEP_H_

namespace j3 {
	class J3Method;

	class J3TrampolineArg {
		/* xmm0 - xmm7 + %rdi/%rsi/%rdx/%rcx/%r8/%r9 + %rbp, %rsp */
		uint64_t  registers[8*2 + 6 + 2]; 
	};
}

#endif
