#ifndef _J3_MANGLER_H_
#define _J3_MANGLER_H_

#include <stdint.h>

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3ObjectType;
	class J3Method;

	class J3Mangler {
		static const uint32_t max = 65536;

		J3ObjectType* from;
		char          buf[max];
		char*         cur;
		char*         next;

		void          check(uint32_t n);

	public:
		static const char*    j3Id;
		static const char*    javaId;

		J3Mangler(J3ObjectType* from);

		char*    cStr() { return buf; }
		uint32_t length() { return cur - buf; }

		J3Mangler* mangle(const char* prefix);
		J3Mangler* mangle(const vmkit::Name* name);
		J3Mangler* mangle(J3Method* method);
		J3Mangler* mangleType(J3Method* method);
	};
}

#endif
