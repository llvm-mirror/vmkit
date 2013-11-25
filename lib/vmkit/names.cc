#include <string.h>

#include "vmkit/names.h"

using namespace vmkit;

T_ptr_less_t<const Name*> Name::less;

Name::Name(uint32_t length, const wchar_t* content) {
	_length = length;
	memcpy(_content, content, sizeof(wchar_t)*(length+1));
}

void* Name::operator new(size_t unused, BumpAllocator* allocator, size_t n) {
	return PermanentObject::operator new(sizeof(Name) + n * sizeof(wchar_t), allocator);
}

Names::Names(BumpAllocator* _allocator) : names(Util::wchar_t_less, _allocator) {
	pthread_mutex_init(&mutex, 0);
	allocator = _allocator;
}

const Name* Names::get(const wchar_t* str) {
	pthread_mutex_lock(&mutex);
	//printf("---- internalize %ls\n", str);

	const Name* res;
	std::map<const wchar_t*, const Name*>::iterator it = names.find(str);

	if(it == names.end()) {
		size_t len = wcslen(str);
		Name* tmp = new(allocator, len) Name(len, str);

		names[tmp->cStr()] = tmp;

		res = tmp;
	} else {
		res = it->second;
	}
	pthread_mutex_unlock(&mutex);
	return res;
}

const Name* Names::get(const char* str, size_t start, size_t length) {
	//printf("%s %lu %lu\n", str, start, length);
	if(length == -1)
		length = strlen(str);
	wchar_t  buf[length + 1];
	size_t   n = 0;
	size_t   i = 0;

	while (i < length) {
		wchar_t x = str[i++];
		if(x & 0x80) {
			wchar_t y = str[i++];
			if (x & 0x20) {
				wchar_t z = str[i++];
				x = ((x & 0x0F) << 12) +
					((y & 0x3F) << 6) +
					(z & 0x3F);
			} else {
				x = ((x & 0x1F) << 6) +
					(y & 0x3F);
			}
		}
		buf[n++] = x;
	}

	buf[n] = 0;

	return get(buf);
}

const Name* Names::get(char c) {
	wchar_t buf[2];
	buf[0] = c;
	buf[1] = 0;
	return get(buf);
}
