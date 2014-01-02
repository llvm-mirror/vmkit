#ifndef _J3_SIGNATURE_H_
#define _J3_SIGNATURE_H_

#include "vmkit/allocator.h"

namespace llvm {
	class FunctionType;
	class Module;
}

namespace j3 {
	class J3MethodType;
	class J3Type;
	class J3Value;
	class J3CodeGen;

	class J3LLVMSignature : vmkit::PermanentObject {
		friend class J3CodeGen;

	public:
		typedef J3Value (*function_t)(void* fn, J3Value* args);

	private:
		llvm::FunctionType* _functionType;
		function_t          _caller;

		void                generateCallerIR(J3CodeGen* vm, llvm::Module* module, const char* id);

		J3LLVMSignature(llvm::FunctionType* functionType);

		llvm::FunctionType* functionType() { return _functionType; }
	public:
		function_t          caller() { return _caller; }
	};
}

#endif
