#ifndef _J3_LIB_H_
#define _J3_LIB_H_

#include <vmkit/allocator.h>
#include <vector>

namespace j3 {
	class J3;
	class J3ClassBytes;

	class J3Lib {
	public:
		static const char*  systemClassesArchives();
		static const char*  systemLibraryPath();
		static int          loadSystemLibraries(std::vector<void*, vmkit::StdAllocator<void*> >* handles);

		static void         bootstrap(J3* vm);
	};
}

#endif
