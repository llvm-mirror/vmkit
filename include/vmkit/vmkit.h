#ifndef _VMKIT_VM_H_
#define _VMKIT_VM_H_

#include "vmkit/util.h"
#include "vmkit/allocator.h"

#include "llvm/ExecutionEngine/JITEventListener.h"

namespace llvm {
	class LLVMContext;
	class Module;
	class DataLayout;
	class GlobalValue;
	class Function;
	class Type;
	namespace legacy {
		class FunctionPassManager;
	}
	using legacy::FunctionPassManager;
}

namespace vmkit {
	class Thread;

	class Safepoint { /* managed with malloc/free */
		const llvm::Function* _llvmFunction;
		uintptr_t             _address;
		size_t                _nbSlots;
		uintptr_t             _offsets[1];

		void* operator new(size_t unused, size_t nbSlots);
		Safepoint(const llvm::Function* llvmFunction, uintptr_t address, size_t nbSlots);

	public:
		static Safepoint* create(const llvm::Function* llvmFunction, uintptr_t address, size_t nbSlots);

		void setAt(size_t idx, uintptr_t offset)  { _offsets[idx] = idx; }
	};

	class ExceptionDescriptor { /* managed with malloc/free */
		const llvm::Function* _llvmFunction;
		uintptr_t             _point;
		uintptr_t             _landingPad;
	public:
		ExceptionDescriptor(const llvm::Function* llvmFunction, uintptr_t point, uintptr_t landingPad);
	};

	class VMKit : public llvm::JITEventListener {
		typedef std::map<const char*, llvm::GlobalValue*, Util::char_less_t, 
										 StdAllocator<std::pair<const char*, llvm::GlobalValue*> > > MangleMap;

		std::map<uintptr_t, Safepoint*>            safepointMap;   /* managed with malloc/free */
		std::map<uintptr_t, ExceptionDescriptor*>  exceptionTable; /* managed with malloc/free */
		MangleMap                       mangleMap;
		BumpAllocator*                  _allocator;
		llvm::Module*                   _self;
		llvm::DataLayout*               _dataLayout;
		void*                           ptrTypeInfo;

		void                       addSymbol(llvm::GlobalValue* gv);

		static void                defaultInternalError(const wchar_t* msg, va_list va) __attribute__((noreturn));
	protected:
		void* operator new(size_t n, BumpAllocator* allocator);

	public:
		static void                destroy(VMKit* vm);

		VMKit(BumpAllocator* allocator);

		BumpAllocator*             allocator() { return _allocator; }

		void                       vmkitBootstrap(Thread* initialThread, const char* selfBitCodePath);

		llvm::DataLayout*          dataLayout() { return _dataLayout; }
		llvm::LLVMContext&         llvmContext();
		llvm::Module*              self() { return _self; }
		llvm::FunctionPassManager* preparePM(llvm::Module* mod);
		llvm::Function*            getGCRoot(llvm::Module* mod);
		llvm::Function*            introspectFunction(llvm::Module* dest, const char* name);
		llvm::GlobalValue*         introspectGlobalValue(llvm::Module* dest, const char* name);
		llvm::Type*                introspectType(const char* name);

		void log(const wchar_t* msg, ...);

		virtual void internalError(const wchar_t* msg, va_list va) __attribute__((noreturn));

		static void internalError(const wchar_t* msg, ...) __attribute__((noreturn));		
		static void throwException(void* obj) __attribute__((noreturn));

		void NotifyFunctionEmitted(const llvm::Function &F,
															 void *Code,
															 size_t Size,
															 const llvm::JITEventListener::EmittedFunctionDetails &Details);

	};
};

#endif
