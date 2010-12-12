#include "P3Extractor.h"
#include "P3Reader.h"
#include "P3Error.h"

using namespace p3;

#define TYPE_NULL               '0'
#define TYPE_NONE               'N'
#define TYPE_FALSE              'F'
#define TYPE_TRUE               'T'
#define TYPE_STOPITER           'S'
#define TYPE_ELLIPSIS           '.'
#define TYPE_INT                'i'
#define TYPE_INT64              'I'
#define TYPE_FLOAT              'f'
#define TYPE_BINARY_FLOAT       'g'
#define TYPE_COMPLEX            'x'
#define TYPE_BINARY_COMPLEX     'y'
#define TYPE_LONG               'l'
#define TYPE_STRING             's'
#define TYPE_INTERNED           't'
#define TYPE_STRINGREF          'R'
#define TYPE_TUPLE              '('
#define TYPE_LIST               '['
#define TYPE_DICT               '{'
#define TYPE_CODE               'c'
#define TYPE_UNICODE            'u'
#define TYPE_UNKNOWN            '?'
#define TYPE_SET                '<'
#define TYPE_FROZENSET          '>'

void P3Extractor::readObject() {
	uint8 type = reader->readU1();

	printf("reading object with type: %d (%c)\n", type, type);

	switch(type) {
		case TYPE_NONE:
			break;

		case TYPE_TUPLE:
			{
				uint32 length = reader->readU4();
				printf("  tuple length: %d\n", length);
				for(uint32 i=0; i<length; i++)
					readObject();
			}
			break;

		case TYPE_INTERNED:
		case TYPE_STRING:
			{
				uint32 length = reader->readU4();
				printf("  string length: %d\n", length);
				if(length > INT_MAX)
					fatal("wrong length for string");
				for(uint32 i=0; i<length; i++) {
					uint8 c = reader->readU1();
					printf("    %-3d (%c) ", c, c);
				}
				printf("\n");
			}
			break;

		case TYPE_CODE:
			{
				uint32 nargs   = reader->readU4();
				uint32 nlocals = reader->readU4();
				uint32 ssize   = reader->readU4();
				uint32 flags   = reader->readU4();
				readObject();      // code
				readObject();      // const
				readObject();      // names
				readObject();      // varnames
				readObject();      // freevars
				readObject();      // cellvars
				readObject();      // filename
				readObject();      // name
				reader->readU4();  // line num
				readObject();      // lnotab
			}
			break;

		default:
			fatal("wrong type info: %d ('%c')", type, type);
	}
}
