#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "allocator.h"
#include "util.h"

namespace llvm {
	class Module;
	class ExecutionEngine;
	class GlobalValue;

	namespace legacy {
		class PassManager;
	}
	using legacy::PassManager;

};

namespace vmkit {
	class VMKit;

	class Symbol : public PermanentObject {
	public:
		virtual void* getSymbolAddress();
	};

	class NativeSymbol : public Symbol {
		void* addr;
	public:
		NativeSymbol(void* _addr) { addr = _addr; }

		void* getSymbolAddress() { return addr; }
	};

	class CompilationUnit  : public llvm::SectionMemoryManager {
		typedef std::map<const char*, Symbol*, Util::char_less_t, StdAllocator<std::pair<const char*, Symbol*> > > SymbolMap;

		VMKit*                  _vmkit;
		BumpAllocator*          _allocator;
		SymbolMap               _symbolTable;
		pthread_mutex_t         _mutexSymbolTable;
		llvm::ExecutionEngine*  _ee;
		llvm::ExecutionEngine*  _oldee;
		llvm::PassManager*      pm;

	protected:
		void  operator delete(void* self);

	public:
		void* operator new(size_t n, BumpAllocator* allocator);

		CompilationUnit(BumpAllocator* allocator, VMKit* vmkit, const char* id);
		~CompilationUnit();

		static void destroy(CompilationUnit* unit);

		VMKit*                  vm() const { return _vmkit; }

		void                    addSymbol(const char* id, vmkit::Symbol* symbol);
		Symbol*                 getSymbol(const char* id);
		uint64_t                getSymbolAddress(const std::string &Name);

		BumpAllocator*          allocator() { return _allocator; }
		llvm::ExecutionEngine*  ee() { return _ee; }
		llvm::ExecutionEngine*  oldee() { return _oldee; }

		void                    compileModule(llvm::Module* module);
	};
}

#endif
