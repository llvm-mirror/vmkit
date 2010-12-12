#ifndef _P3_OBJECT_H_
#define _P3_OBJECT_H_

#include <types.h>
#include <stdint.h>

#include "P3Error.h"

namespace p3 {

class P3String;
class P3Tuple;
class P3Code;
class P3None;

class  P3Object {
public:
#define do_type(type, name)												\
	virtual bool is##name() { return 0; }						\
	type*        as##name() { if(!is##name()) fatal("%p is not a "#name, (void*)this); return (type*)this; }

	do_type(P3String, String)
	do_type(P3Tuple,  Tuple)
	do_type(P3None,   None)
	do_type(P3Code,   Code)

#undef do_type

	virtual void print() { printf("object@%p", (void*)this); }
};

class P3None : public P3Object {
public:
	virtual void print() { printf("<none>"); }
};

class P3String : public P3Object {
public:
	uint32 length;
	uint8* content;

	P3String(uint32 n);

	virtual bool isString() { return 1; }
	virtual void print() { printf("%s", content); }
};

class P3Tuple : public P3Object {
public:
	uint32     length;
	P3Object** content;

	P3Tuple(uint32 n);

	P3Object* at(uint32 idx) { 
		if(idx >= length)
			fatal("array out of bound: %d", idx);
		return content[idx]; 
	}

	virtual bool isTuple() { return 1; }

	virtual void print() { printf("["); for(uint32 i=0; i<length; i++) content[i]->print(); printf("]"); }
};

class P3Stack : public P3Tuple {
public:
	uint32 capacity;

	P3Stack(uint32 c) : P3Tuple(c) { length = 0; capacity = c; }
};

class P3Code : public P3Object {
public:
	uint32    py_nargs;
	uint32    py_nlocals;
	uint32    py_nstacks;
	uint32    py_flag;
	P3String* py_code;
	P3Tuple*  py_const;
	P3Object* py_names;
	P3Object* py_varnames;
	P3Object* py_freevars;
	P3Object* py_cellvars;
	P3Object* py_filename;
	P3Object* py_name;
	uint32    py_linenum;
	P3Object* py_lnotab;

	virtual bool isCode() { return 1; }

	virtual void print() { printf("Code<"); py_name->print(); printf(">"); }
};

} // namespace p3

#endif
