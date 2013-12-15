#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "allocator.h"
#include "util.h"

namespace llvm {
	class Module;
	class ExecutionEngine;

	namespace legacy {
		class PassManager;
	}
	using legacy::PassManager;

};

namespace vmkit {
	class Symbol : public PermanentObject {
	public:
		virtual uint8_t* getSymbolAddress();
	};

	class NativeSymbol : public Symbol {
		uint8_t* addr;
	public:
		NativeSymbol(uint8_t* _addr) { addr = _addr; }

		uint8_t* getSymbolAddress() { return addr; }
	};

	class CompilationUnit  : public llvm::SectionMemoryManager {
		typedef std::map<const char*, Symbol*, Util::char_less_t, StdAllocator<std::pair<const char*, Symbol*> > > SymbolMap;

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

		CompilationUnit(BumpAllocator* allocator, const char* id);
		~CompilationUnit();

		static void destroy(CompilationUnit* unit);

		void                    addSymbol(const char* id, vmkit::Symbol* symbol);
		uint64_t                getSymbolAddress(const std::string &Name);

		BumpAllocator*          allocator() { return _allocator; }
		llvm::ExecutionEngine*  ee() { return _ee; }
		llvm::ExecutionEngine*  oldee() { return _oldee; }

		void                    addModule(llvm::Module* module);
	};
}

#endif
