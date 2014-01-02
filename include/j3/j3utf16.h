#ifndef _J3_UTF16_H_
#define _J3_UTF16_H_

#include <sys/types.h>
#include <stdint.h>

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3ObjectHandle;

	class J3Utf16Converter {
		const vmkit::Name* name;
		size_t             pos;
	public:
		J3Utf16Converter(const vmkit::Name* _name);

		bool isEof();
		uint16_t nextUtf16();
	};

	class J3CharConverter {
	public:
		static size_t maxSize(J3ObjectHandle* charArray);
		static size_t convert(J3ObjectHandle* charArray, char* dest);
	};
}

#endif
