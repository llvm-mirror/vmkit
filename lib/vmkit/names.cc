#include <string.h>
#include <wchar.h>

#include "vmkit/names.h"

using namespace vmkit;

T_ptr_less_t<const Name*> Name::less;

Name::Name(size_t length, const char* content) {
	_length = length;
	memcpy(_content, content, sizeof(char)*length);
	_content[length] = 0;
}

void* Name::operator new(size_t unused, BumpAllocator* allocator, size_t n) {
	return PermanentObject::operator new(sizeof(Name) + n * sizeof(uint8_t), allocator);
}

Names::Names(BumpAllocator* _allocator) : names(Util::char_less, _allocator) {
	pthread_mutex_init(&mutex, 0);
	allocator = _allocator;
}

const Name* Names::get(const char* str, size_t length) {
	pthread_mutex_lock(&mutex);
	//printf("---- internalize %s\n", str);

	const Name* res;
	std::map<const char*, const Name*>::iterator it = names.find(str);

	if(it == names.end()) {
		Name* tmp = new(allocator, length) Name(length, str);

		names[tmp->cStr()] = tmp;

		res = tmp;
	} else {
		res = it->second;
	}
	pthread_mutex_unlock(&mutex);
	return res;
}

const Name* Names::get(const char* str) {
	return get(str, strlen(str));
}

const Name* Names::get(const char* str, size_t start, size_t length) {
	//printf("%s %lu %lu\n", str, start, length);
	if(length == -1)
		return get(str+start);
	else {
		char buf[length+1];
		memcpy(buf, str+start, length);
		buf[length] = 0;
		return get(buf, length);
	}
}

#if 0
		char buf[length + 1];
		size_t   n = 0;
		size_t   i = 0;

		while (i < length) {
			char x = str[i++];
			if(x & 0x80) {
				char y = str[i++];
				if (x & 0x20) {
					char z = str[i++];
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
#endif

const Name* Names::get(char c) {
	char buf[2];
	buf[0] = c;
	buf[1] = 0;
	return get(buf);
}
