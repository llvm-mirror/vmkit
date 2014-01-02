#ifndef _J3_UTF16_H_
#define _J3_UTF16_H_

namespace j3 {
	class J3Utf16Converter {
		const vmkit::Name* name;
		size_t             pos;
	public:
		J3Utf16Converter(const vmkit::Name* _name) { name = _name; pos = 0; }

		bool isEof() { return pos == name->length(); }

		uint16_t nextUtf16() {
			const char* str = name->cStr();
			size_t   n = 0;
			size_t   i = 0;
			uint16_t x = str[pos++];

			if(x & 0x80) {
				uint16_t y = str[pos++];
				if (x & 0x20) {
					char z = str[pos++];
					x = ((x & 0x0F) << 12) +
						((y & 0x3F) << 6) +
						(z & 0x3F);
				} else {
					x = ((x & 0x1F) << 6) +
						(y & 0x3F);
				}
			}
		
			return x;
		}
	};
}

#endif
