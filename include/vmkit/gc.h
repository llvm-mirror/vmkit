#ifndef _GC_H_
#define _GC_H_

#include <sys/types.h>

namespace vmkit {
	class GC {
	public:
		static void* allocate(size_t sz);
	};
};

#endif
