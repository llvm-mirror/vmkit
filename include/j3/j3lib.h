#ifndef _J3_LIB_H_
#define _J3_LIB_H_

#include <vmkit/allocator.h>
#include <vector>

namespace j3 {
	class J3;
	class J3ClassLoader;

	class J3Lib {
	public:
		static const char*  systemClassesArchives();
		static const char*  systemLibraryPath();
		static const char*  extDirs();
		static void         loadSystemLibraries(J3ClassLoader* loader);

		static void         bootstrap(J3* vm);
	};
}

#endif
