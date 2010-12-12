#include "P3Extractor.h"
#include "P3Reader.h"
#include "P3Error.h"
#include "P3Object.h"

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

P3Object* P3Extractor::readObject() {
	uint8 type = reader->readU1();

	switch(type) {
		case TYPE_NONE:
			return new P3None();

		case TYPE_TUPLE:
			{
				uint32 length = reader->readU4();
				P3Tuple* res = new P3Tuple(length);
				for(uint32 i=0; i<length; i++)
					res->content[i] = readObject();
				return res;
			}

		case TYPE_INTERNED:
		case TYPE_STRING:
			{
				uint32 length = reader->readU4();
				P3String* res = new P3String(length);
				if(length > INT_MAX)
					fatal("wrong length for string");
				for(uint32 i=0; i<length; i++)
					res->content[i] = reader->readU1();
				return res;
			}

		case TYPE_CODE:
			{
				P3Code* res = new P3Code();
				res->py_nargs    = reader->readU4();
				res->py_nlocals  = reader->readU4();
				res->py_nstacks  = reader->readU4();
				res->py_flag     = reader->readU4();
				res->py_code     = readObject()->asString();
				res->py_const    = readObject()->asTuple();
				res->py_names    = readObject();
				res->py_varnames = readObject();
				res->py_freevars = readObject();
				res->py_cellvars = readObject();
				res->py_filename = readObject();
				res->py_name     = readObject();
				res->py_linenum  = reader->readU4();
				res->py_lnotab   = readObject();
				return res;
			}

		default:
			fatal("wrong type info: %d ('%c')", type, type);
	}
}
