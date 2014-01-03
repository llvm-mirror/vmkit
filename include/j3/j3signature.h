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
		J3ClassLoader*               _loader;
		const vmkit::Name*           _name;
		J3LLVMSignature*             _staticLLVMSignature;
		J3LLVMSignature*             _virtualLLVMSignature;
		J3Type* volatile             _out;
		uint32_t                     _nbIns;
		J3Type**                     _ins;
		llvm::FunctionType*          _staticFunctionType;
		llvm::FunctionType* volatile _virtualFunctionType;

		void checkInOut();
		void checkFunctionType();
	public:
		J3Signature(J3ClassLoader* loader, const vmkit::Name* name);

		const vmkit::Name*  name() { return _name; }
		J3ClassLoader*      loader() { return _loader; }
		llvm::FunctionType* functionType(uint32_t access); /* has to be called when compiler lock is held */
		void                setLLVMSignature(uint32_t access, J3LLVMSignature* llvmSignature);
		J3LLVMSignature*    llvmSignature(uint32_t access);
		J3Type*             out() { checkInOut(); return _out; }
		uint32_t            nbIns() { checkInOut(); return _nbIns; }
		J3Type*             ins(uint32_t idx) { checkInOut(); return _ins[idx]; }
	};

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
