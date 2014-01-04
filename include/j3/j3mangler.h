#ifndef _J3_MANGLER_H_
#define _J3_MANGLER_H_

#include <stdint.h>
#include <sys/types.h>

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3Class;
	class J3Method;
	class J3Signature;

	class J3Mangler {
		static const size_t max = 65536;

		J3Class* from;
		char     buf[max];
		char*    cur;
		char*    next;

		void     check(size_t n);
		char     hex(uint32_t n);
	public:
		static const char*    j3Id;
		static const char*    javaId;

		J3Mangler(J3Class* from);

		char*    cStr() { return buf; }
		size_t   length() { return cur - buf; }

		J3Mangler* mangle(uint16_t utf16);
		J3Mangler* mangle(const char* prefix);
		J3Mangler* mangle(const char* prefix, size_t length);
		J3Mangler* mangle(const vmkit::Name* name);
		J3Mangler* mangle(J3Signature* signature);
		J3Mangler* mangle(const vmkit::Name* clName, const vmkit::Name* name);
	};
}

#endif
