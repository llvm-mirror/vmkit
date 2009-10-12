//===----------------- VMArray.cpp - VM arrays ------------------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>

#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"
#include "N3.h"


using namespace n3;

const sint32 VMArray::MaxArraySize = 268435455;

#define PRINT(name, printer, pre, sep, post)														\
	void name::print(mvm::PrintBuffer *buf) const	{												\
		declare_gcroot(const name *, self) = this;													\
	  buf->write(pre);																										\
	  for(int i=0; i<self->size; i++) {																		\
	    if(i)																															\
				buf->write(sep);																								\
			buf->printer(self->elements[i]);																	\
		}																																		\
		buf->write(post);																										\
	}

#define ARRAYCLASS(name, elmt, size, printer, pre, sep, post)						\
	PRINT(name, printer, pre, sep, post)

ARRAYCLASS(ArrayUInt8,  uint8,     1, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArraySInt8,  sint8,     1, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArrayChar,   uint16,    2, writeChar, "",       "",  "")
ARRAYCLASS(ArrayUInt16, uint16,    2, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArraySInt16, sint16,    2, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArrayUInt32, uint32,    4, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArraySInt32, sint32,    4, writeS4,   "Array<", " ", ">")
ARRAYCLASS(ArrayLong,   sint64,    8, writeS8,   "Array<", " ", ">")
ARRAYCLASS(ArrayFloat,  float,     4, writeFP,   "Array<", " ", ">")
ARRAYCLASS(ArrayDouble, double,    8, writeFP,   "Array<", " ", ">")
ARRAYCLASS(ArrayObject, VMObject*, 4, writeObj,  "Array<", " ", ">")

void VMArray::print(mvm::PrintBuffer *buf) const {
  assert(0 && "should not be here");
}

#undef PRINT
#undef AT
#undef INITIALISE
#undef ACONS
#undef ARRAYCLASS
