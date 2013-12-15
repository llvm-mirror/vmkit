#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "allocator.h"

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
		llvm::Module*   _module;
	public:
		CompilationFragment();
	};

	class CompilationUnit  : public llvm::SectionMemoryManager {
		typedef std::map<const char*, Symbol*, vmkit::Util::char_less_t, StdAllocator<std::pair<const char*, Symbol*> > > SymbolMap;
	};
#endif
}

#endif
