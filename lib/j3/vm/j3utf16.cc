#include "j3/j3utf16.h"
#include "j3/j3object.h"
#include "j3/j3.h"
#include "vmkit/names.h"

using namespace j3;

J3Utf16Encoder::J3Utf16Encoder(const vmkit::Name* _name) { 
	name = _name; 
	pos = 0; 
}

bool J3Utf16Encoder::isEof() { 
	return pos == name->length(); 
}

uint16_t J3Utf16Encoder::nextUtf16() {
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

size_t J3Utf16Decoder::decode(J3ObjectHandle* charArray, char* dest) {
	size_t length = charArray->arrayLength();
	size_t pos = 0;

	for(uint32_t i=0; i<length; i++) {
		uint16_t c = charArray->getCharAt(i);
		if(c > 127) {
			J3::internalError("implement me: fun char");
		} else {
			dest[pos++] = (char)c;
		}
	}

	dest[pos] = 0;

	return pos;
}
		
size_t J3Utf16Decoder::maxSize(J3ObjectHandle* charArray) {
	return 1 + charArray->arrayLength() * 3;
}


