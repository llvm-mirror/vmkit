#ifndef _J3_SYMBOL_H_
#define _J3_SYMBOL_H_

#include "vmkit/allocator.h"

namespace j3 {
	class J3Symbol : public vmkit::PermanentObject {
	public:
		uint64_t getSymbolAddress();
	};
};

#endif
