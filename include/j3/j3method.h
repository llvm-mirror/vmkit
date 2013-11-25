#ifndef _J3_METHOD_H_
#define _J3_METHOD_H_

#include <stdint.h>
#include <vector>

#include "vmkit/allocator.h"

namespace llvm {
	class FunctionType;
	class Function;
	class Type;
	class GlobalValue;
	class Module;
	struct GenericValue;
}

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3Type;
	class J3Attributes;
	class J3Class;
	class J3Method;
	class J3Value;
	class J3ObjectHandle;

	class J3MethodType : public vmkit::PermanentObject {
		llvm::FunctionType* _llvmType;
		J3Type*             _out;
		uint32_t            _nbIns;
		J3Type*             _ins[1];

	public:
		J3MethodType(J3Type** args, size_t nbArgs);

		llvm::FunctionType* llvmType() { return _llvmType; }
		uint32_t            nbIns() { return _nbIns; }
		J3Type*             out() { return _out; }
		J3Type*             ins(uint32_t idx) { return _ins[idx]; }

		void* operator new(size_t unused, vmkit::BumpAllocator* allocator, size_t n) {
			return vmkit::PermanentObject::operator new(sizeof(J3MethodType) + (n - 1) * sizeof(J3Type*), allocator);
		}
	};

	struct  trampolin_t {
		uint8_t         code[1];
	};

	class J3Method : public vmkit::PermanentObject {
	public:
		uint16_t                     _access;
		J3Class*                     _cl;
		const vmkit::Name*           _name;
		const vmkit::Name*           _sign;
		J3MethodType*                _methodType;
		J3Attributes*                _attributes;
		uint32_t                     _index;
		llvm::Function*              _nomcjitFunction;
		llvm::GlobalValue*           _nomcjitDescriptor;
		llvm::Function*              _compiledFunction;
		char* volatile               _llvmAllNames; /* md_ + llvm Name */
		size_t                       _llvmAllNamesLength;
		void*                        _nativeFnPtr;

		uint8_t                      _trampoline[1];

		llvm::Type* doNativeType(J3Type* type);

		J3Value            internalInvoke(bool statically, std::vector<llvm::GenericValue>* args);
		J3Value            internalInvoke(bool statically, J3ObjectHandle* handle, va_list va);
		J3Value            internalInvoke(bool statically, J3ObjectHandle* handle, J3Value* args);

		void*              operator new(size_t size, vmkit::BumpAllocator* allocator, size_t trampolineSize);

		void               buildLLVMNames(J3Class* from);
	public:
		J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, const vmkit::Name* sign);

		static J3Method*    newMethod(vmkit::BumpAllocator* allocator, 
																	uint16_t access, 
																	J3Class* cl, 
																	const vmkit::Name* name, 
																	const vmkit::Name* sign);

		size_t              llvmFunctionNameLength(J3Class* from=0);
		char*               llvmFunctionName(J3Class* from=0);
		char*               llvmDescriptorName(J3Class* from=0);
		llvm::FunctionType* llvmType(J3Class* from=0);

		void                postInitialise(uint32_t access, J3Attributes* attributes);
		void                setResolved(uint32_t index); 

		J3Method*           resolve(J3ObjectHandle* obj);

		llvm::Function*     nativeLLVMFunction(llvm::Module* module);
		llvm::GlobalValue*  llvmDescriptor(llvm::Module* module);
		llvm::Function*     llvmFunction(bool isStub, llvm::Module* module, J3Class* from=0);

		uint32_t            index();
		uint32_t*           indexPtr() { return &_index; }
		bool                isResolved() { return _index != -1; }

		J3Attributes*       attributes() const { return _attributes; }
		uint16_t            access() const { return _access; }
		J3Class*            cl()     const { return _cl; }
		const vmkit::Name*  name()   const { return _name; }
		const vmkit::Name*  sign()   const { return _sign; }
		J3MethodType*       methodType(J3Class* from=0);

		void                registerNative(void* ptr);

		J3Value             invokeStatic(...);
		J3Value             invokeStatic(J3Value* args);
		J3Value             invokeStatic(va_list va);
		J3Value             invokeSpecial(J3ObjectHandle* obj, ...);
		J3Value             invokeSpecial(J3ObjectHandle* obj, J3Value* args);
		J3Value             invokeSpecial(J3ObjectHandle* obj, va_list va);
		J3Value             invokeVirtual(J3ObjectHandle* obj, ...);
		J3Value             invokeVirtual(J3ObjectHandle* obj, J3Value* args);
		J3Value             invokeVirtual(J3ObjectHandle* obj, va_list va);

		void*               fnPtr();
		void*               functionPointerOrTrampoline();

		void                dump();
	};
}

#endif
