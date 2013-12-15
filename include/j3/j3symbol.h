#ifndef _J3_SYMBOL_H_
#define _J3_SYMBOL_H_

#include "vmkit/allocator.h"

namespace j3 {
	class J3Symbol : public vmkit::PermanentObject {
	public:
		virtual uint8_t* getSymbolAddress();
	};

	class J3NativeSymbol : public J3Symbol {
		uint8_t* addr;
	public:
		J3NativeSymbol(uint8_t* _addr) { addr = _addr; }

		uint8_t* getSymbolAddress() { return addr; }
	};
};

#endif
