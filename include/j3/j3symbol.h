#ifndef _J3_SYMBOL_H_
#define _J3_SYMBOL_H_

#include "vmkit/allocator.h"

namespace j3 {
	class J3Symbol : public vmkit::PermanentObject {
	public:
		virtual int isMethodPointer() { return 0; }
		virtual int isMethodDescriptor() { return 0; }
		virtual int isTypeDescriptor() { return 0; }
	};
};

#endif
