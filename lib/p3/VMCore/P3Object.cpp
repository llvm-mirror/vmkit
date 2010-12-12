#include "P3Object.h"

using namespace p3;

P3String::P3String(uint32 l) {
	content = new uint8[l+1];
	length  = l;
	content[l] = 0;
}

P3Tuple::P3Tuple(uint32 l) {
	content = new P3Object*[l];
	length  = l;
}
