#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "allocator.h"
#include "util.h"

namespace llvm {
	class Module;
	class ExecutionEngine;
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

#if 0
	class CompilationFragment {
		BumpAllocator*  _allocator;
		llvm::Module*   _module;

	public:
		CompilationFragment(BumpAllocator* allocator);

		BumpAllocator* allocator() { return _allocator; }
		llvm::Module*  module() { return _module; }
	};
#endif

	class CompilationUnit  : public llvm::SectionMemoryManager {
		typedef std::map<const char*, Symbol*, Util::char_less_t, StdAllocator<std::pair<const char*, Symbol*> > > SymbolMap;

		SymbolMap               _symbolTable;
		pthread_mutex_t         _mutexSymbolTable;
		llvm::ExecutionEngine*  _ee;
		llvm::ExecutionEngine*  _oldee;

	public:
		llvm::ExecutionEngine*  ee() { return _ee; }
		llvm::ExecutionEngine*  oldee() { return _oldee; }
	};
}

#endif
