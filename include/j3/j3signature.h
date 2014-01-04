#ifndef _J3_SIGNATURE_H_
#define _J3_SIGNATURE_H_

#include "vmkit/allocator.h"

namespace vmkit {
	class Name;
}

namespace llvm {
	class FunctionType;
	class Module;
}

namespace j3 {
	class J3ClassLoader;
	class J3LLVMSignature;
	class J3Type;
	class J3Value;
	class J3CodeGen;

	class J3Signature : public vmkit::PermanentObject {
	public:
		typedef J3Value (*function_t)(void* fn, J3Value* args);

	private:
		J3ClassLoader*               _loader;
		const vmkit::Name*           _name;
		J3LLVMSignature*             _staticLLVMSignature;
		J3LLVMSignature* volatile    _virtualLLVMSignature;
		J3Type* volatile             _out;
		J3Type**                     _ins;

		J3LLVMSignature*    buildLLVMSignature(llvm::FunctionType* fType);
		J3LLVMSignature*    llvmSignature(uint32_t access);
		
		void checkInOut();
		void checkFunctionType();
	public:
		J3Signature(J3ClassLoader* loader, const vmkit::Name* name);

		const vmkit::Name*  name() { return _name; }
		J3ClassLoader*      loader() { return _loader; }
		J3Type*             javaOut() { checkInOut(); return _out; }
		J3Type*             javaIns(uint32_t idx) { checkInOut(); return _ins[idx]; }
		uint32_t            nbIns();

		function_t          caller(uint32_t access);

		/* only call this function outside the compiler when the functionType already exists */
		llvm::FunctionType* functionType(uint32_t access);

		/* only call the remaining functions while the compiler lock is held */
		void                generateCallerIR(uint32_t access, J3CodeGen* codeGen, llvm::Module* module, const char* id);
		void                setCaller(uint32_t access, J3Signature::function_t caller);
	};

	class J3LLVMSignature : public vmkit::PermanentObject {
	public:
		llvm::FunctionType*     functionType;
		J3Signature::function_t caller;
	};
}

#endif
