#include "j3/j3mangler.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"
#include "j3/j3utf16.h"
#include "j3/j3.h"

using namespace j3;

const char*    J3Mangler::j3Id = "j3_";
const char*    J3Mangler::javaId = "Java_";


J3Mangler::J3Mangler(J3Class* _from) {
	from = _from;
	cur  = buf;
}

void J3Mangler::check(uint32_t n) {
	next = cur + n;
	if((next+1) >= (buf + max))
		J3::internalError("unable to mangle: not enough space");
}

J3Mangler* J3Mangler::mangleType(J3Method* method) {
	J3MethodType* type = method->methodType(from);

	if(type->nbIns()) {
		check(2);
		cur[0] = '_';
		cur[1] = '_';
		cur = next;
		
		for(uint32_t i=0; i<type->nbIns(); i++) {
			check(type->ins(i)->nativeNameLength());
			memcpy(cur, type->ins(i)->nativeName(), type->ins(i)->nativeNameLength());
			cur = next;
		}
	}
	check(1);
	*cur = 0;
	
	return this;
}

J3Mangler* J3Mangler::mangle(J3Method* method) {
	check(method->cl()->nativeNameLength() - 3);
	memcpy(cur, method->cl()->nativeName() + 1, method->cl()->nativeNameLength() - 3);
	*next = '_';
	cur = next+1;

	mangle(method->name());

	return this;
}

J3Mangler* J3Mangler::mangle(const char* prefix) {
	uint32_t length = strlen(prefix);

	check(length);
	memcpy(cur, prefix, length);
	*next = 0;
	cur = next;

	return this;
}

J3Mangler* J3Mangler::mangle(const vmkit::Name* name) {
	J3Utf16Encoder encoder(name);

	next = cur;
	while(!encoder.isEof()) {
		uint16_t c = encoder.nextUtf16();

		if(c > 256) {
			check(6);
			*cur++ = '_';
			*cur++ = '0';
			*cur++ = (c >> 24 & 0xf) + '0';
			*cur++ = (c >> 16 & 0xf) + '0';
			*cur++ = (c >> 8  & 0xf) + '0';
			*cur++ = (c >> 0  & 0xf) + '0';
		} else {
			switch(c) {
				case '<':
				case '>':
				case '(':
				case ')':
					break;
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
					check(1);
					*cur++ = c;
			}
		}
		cur = next;
	}

	*cur = 0;

	return this;
}

