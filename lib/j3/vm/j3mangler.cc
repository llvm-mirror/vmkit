#include "j3/j3mangler.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"
#include "j3/j3utf16.h"
#include "j3/j3thread.h"
#include "j3/j3.h"

using namespace j3;

const char*    J3Mangler::j3Id = "j3_";
const char*    J3Mangler::javaId = "Java_";


J3Mangler::J3Mangler(J3Class* _from) {
	from = _from;
	cur  = buf;
}

J3Mangler* J3Mangler::mangle(J3Signature* signature) {
	const vmkit::Name* name = signature->name();

	if(name->cStr()[1] == J3Cst::ID_Right)
		return this;

	check(2);
	cur[0] = '_';
	cur[1] = '_';
	cur = next;

	J3Utf16Encoder encoder(name);
	encoder.nextUtf16();
	uint16_t c;

	while(!encoder.isEof() && ((c = encoder.nextUtf16()) != J3Cst::ID_Right)) {
		mangle(c);
	}

	*cur = 0;

	return this;
}

J3Mangler* J3Mangler::mangle(const vmkit::Name* clName, const vmkit::Name* name) {
	mangle(clName);
	check(1);
	*cur = '_';
	cur = next;
	mangle(name);

	return this;
}

J3Mangler* J3Mangler::mangle(const char* prefix) {
	return mangle(prefix, strlen(prefix));
}

J3Mangler* J3Mangler::mangle(const char* prefix, size_t length) {
	check(length);
	memcpy(cur, prefix, length);
	*next = 0;
	cur = next;

	return this;
}

J3Mangler* J3Mangler::mangle(const vmkit::Name* name) {
	J3Utf16Encoder encoder(name);

	while(!encoder.isEof()) {
		mangle(encoder.nextUtf16());
	}

	*cur = 0;

	return this;
}

J3Mangler* J3Mangler::mangle(uint16_t c) {
	switch(c) {
		case '<':
		case '>':
			break; /* do not encode at all */
		case '(':
		case ')':
			J3Thread::get()->vm()->internalError("should not try to encode a special character such as %c", (char)c);
		case '_': 
			check(2);
			*cur++ = '_'; 
			*cur++ = '1'; 
			break;
		case ';': 
			check(2);
			*cur++ = '_'; 
			*cur++ = '2'; 
			break;
		case '[': 
			check(2);
			*cur++ = '_'; 
			*cur++ = '3'; 
			break;
		case '/': 
			c = '_';
		default: 
			if((c >= 'a' && c <= 'z') ||
				 (c >= 'A' && c <= 'Z') ||
				 (c >= '0' && c <= '9') ||
				 (c == '_')) {
				check(1);
				*cur++ = c;
			} else {
				check(6);
				*cur++ = '_';
				*cur++ = '0';
				*cur++ = hex(c >> 12);
				*cur++ = hex(c >> 8);
				*cur++ = hex(c >> 4);
				*cur++ = hex(c >> 0);
			} 
	}

	cur = next;

	return this;
}

char J3Mangler::hex(uint32_t n) {
	n = n & 0xf;
	return n < 10 ? (n + '0') : (n + 'a' - 10);
}

void J3Mangler::check(size_t n) {
	next = cur + n;
	if((next+1) >= (buf + max))
		J3::internalError("unable to mangle: not enough space");
}
