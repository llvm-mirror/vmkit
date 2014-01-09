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
			uint16_t z = str[pos++];
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
		uint16_t c = charArray->getCharacterAt(i);
		if(c < (1<<7)) {
			dest[pos++] = (char)c;
		} else if(c < (1<<11)) {
			dest[pos++] = ((c>>6) & 0x1f) | 0x80;
			dest[pos++] = c & 0x3f;
		} else {
			dest[pos++] = ((c>>12) & 0xf) | (0x80 + 0x20);
			dest[pos++] = (c>>6) & 0x3f;
			dest[pos++] = c & 0x3f;
		}
	}

	dest[pos] = 0;

	return pos;
}
		
size_t J3Utf16Decoder::maxSize(J3ObjectHandle* charArray) {
	return 1 + charArray->arrayLength() * 3;
}


