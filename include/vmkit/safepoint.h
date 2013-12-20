#ifndef _SAFEPOINT_H_
#define _SAFEPOINT_H_

#include <stdint.h>

namespace llvm {
	class Module;
}

namespace vmkit {
	class CompilationUnit;

	class Safepoint { /* directly in the data sections */
		uintptr_t        _addr;
		void*            _code;
		CompilationUnit* _unit;
		uint32_t         _sourceIndex;
		uint32_t         _nbLives;

	public:
		void       setUnit(CompilationUnit* unit) { _unit = unit; }

		uintptr_t  addr() { return _addr; }
		void*      code() { return _code; }
		uint32_t   sourceIndex() { return _sourceIndex; }
		uint32_t   nbLives() { return _nbLives; }
		uint32_t   liveAt(uint32_t i) { return ((uint32_t*)(this + 1))[i]; }

		Safepoint* getNext();
		void       dump();

		static Safepoint* get(CompilationUnit* unit, llvm::Module* module);
	};
}

#endif
