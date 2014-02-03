#ifndef _J3_METHOD_H_
#define _J3_METHOD_H_

#include <stdint.h>
#include <vector>

#include "vmkit/compiler.h"
#include "j3/j3signature.h"

namespace llvm {
	class FunctionType;
	class Function;
	class Linker;
}

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3LLVMSignature;
	class J3Type;
	class J3Attributes;
	class J3ObjectType;
	class J3Class;
	class J3Method;
	class J3Value;
	class J3ObjectHandle;
	class J3Signature;

	class J3Method : public vmkit::Symbol {
	public:
		uint16_t                     _access;
		J3Class*                     _cl;
		const vmkit::Name*           _name;
		J3Signature*                 _signature;
		J3Attributes*                _attributes;
		J3Attributes*                _codeAttributes;
		uint32_t                     _index;
		uint32_t                     _slot;
		llvm::Function*              _llvmFunction;
		void*                        _fnPtr;
		char* volatile               _llvmAllNames; /* stub + _ + native_name */
		void*                        _nativeFnPtr;
		void* volatile               _staticTrampoline;
		void* volatile               _virtualTrampoline;
		J3ObjectHandle* volatile     _javaMethod;

		J3Value            internalInvoke(J3Value* args);
		J3Value            internalInvoke(J3ObjectHandle* handle, va_list va);
		J3Value            internalInvoke(J3ObjectHandle* handle, J3Value* args);
		void               buildLLVMNames(J3Class* from);
	public:
		J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, J3Signature* signature);

		vmkit::CompilationUnit* unit();

		uint32_t                slot() { return _slot; }

		static J3Method*        nativeMethod(J3ObjectHandle* handle);
		J3ObjectHandle*         javaMethod();

		void*                   nativeFnPtr() { return _nativeFnPtr; }

		void                    markCompiled(llvm::Function* llvmFunction, void* fnPtr);

		uint32_t                interfaceIndex();

		void*                   getSymbolAddress();

		char*                   llvmFunctionName(J3Class* from=0);
		char*                   llvmStubName(J3Class* from=0);

		void                    postInitialise(uint32_t access, J3Attributes* attributes);
		void                    setIndex(uint32_t index); 

		J3Method*               resolve(J3ObjectHandle* obj);

		uint32_t                index();

		J3Attributes*           codeAttributes();
		J3Attributes*           attributes() const { return _attributes; }
		uint16_t                access() const { return _access; }
		J3Class*                cl()     const { return _cl; }
		const vmkit::Name*      name()   const { return _name; }
		J3Signature*            signature() const { return _signature; }

		void                    registerNative(void* ptr);

		J3Value                 invokeStatic(...);
		J3Value                 invokeStatic(J3Value* args);
		J3Value                 invokeStatic(va_list va);
		J3Value                 invokeSpecial(J3ObjectHandle* obj, ...);
		J3Value                 invokeSpecial(J3ObjectHandle* obj, J3Value* args);
		J3Value                 invokeSpecial(J3ObjectHandle* obj, va_list va);
		J3Value                 invokeVirtual(J3ObjectHandle* obj, ...);
		J3Value                 invokeVirtual(J3ObjectHandle* obj, J3Value* args);
		J3Value                 invokeVirtual(J3ObjectHandle* obj, va_list va);

		void                    aotSnapshot(llvm::Linker* linker);
		void                    ensureCompiled(uint32_t mode);
		J3Signature::function_t cxxCaller();
		void*                   fnPtr();
		llvm::Function*         llvmFunction() { return _llvmFunction; } /* overwrite vmkit::Symbol */
		uint64_t                inlineWeight();
		void*                   functionPointerOrStaticTrampoline();
		void*                   functionPointerOrVirtualTrampoline();

		void                    dump();
	};
}

#endif
