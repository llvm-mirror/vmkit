#ifndef _NAMES_H_
#define _NAMES_H_

#include <map>

#include "vmkit/allocator.h"
#include "vmkit/util.h"

namespace vmkit {
	class Name : public PermanentObject {
		uint32_t                      _length;
		wchar_t                       _content[1];

	public:
		void* operator new(size_t unused, BumpAllocator* allocator, size_t length);

		Name(uint32_t length, const wchar_t* content);

		const wchar_t* cStr() const {
			return _content;
		}

		uint32_t length() const {
			return _length;
		}

		static T_ptr_less_t<const Name*> less;
	};

	class Names : public PermanentObject {
		BumpAllocator*  allocator;
		pthread_mutex_t mutex;

		std::map<const wchar_t*, const Name*, Util::wchar_t_less_t, 
						 StdAllocator<std::pair<const wchar_t*, const Name*> > > names;

	public:
		Names(BumpAllocator* allocator);

		const Name*   get(const wchar_t* s);
		const Name*   get(const char* s, size_t start=0, size_t length=-1);
		const Name*   get(char c);

	};

	template <class T>
	class NameMap {
	public:
		typedef StdAllocator<std::pair<const Name*, T> > alloc;
		typedef std::map<const Name*, T, T_ptr_less_t<const Name*>, alloc > map;
	};
}

#endif
