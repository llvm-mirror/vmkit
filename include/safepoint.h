#ifndef _SAFE_POINT_H_
#define _SAFE_POINT_H_

namespace vmkit {
	class SafePoint {
		llvm::Function* function; /* safe inside this function */
		uintptr_t       ip;       /* ip of this safepoint */
	public:
	};
}

#endif
