#ifndef _NAMES_H_
#define _NAMES_H_

#include <map>

#include "vmkit/allocator.h"
#include "vmkit/util.h"

namespace vmkit {
	class Name : public PermanentObject {
		size_t  _length;
		char    _content[1];

	public:
		void* operator new(size_t unused, BumpAllocator* allocator, size_t length);

		Name(size_t length, const char* content);

		const char* cStr() const {
			return _content;
		}

		size_t length() const {
			return _length;
		}

		static T_ptr_less_t<const Name*> less;
	};

	class Names : public PermanentObject {
		BumpAllocator*  allocator;
		pthread_mutex_t mutex;

		std::map<const char*, const Name*, Util::char_less_t, 
						 StdAllocator<std::pair<const char*, const Name*> > > names;

		const Name*   get(const char* s, size_t length);
	public:
		Names(BumpAllocator* allocator);

		const Name*   get(const char* s);
		const Name*   get(const char* s, size_t start, size_t length);
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
