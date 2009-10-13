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

#define DEFINE_ARRAY_PRINT(name, elmt, nbb, printer, pre, sep, post)		\
	void Array##name::do_print(const Array##name *self, mvm::PrintBuffer *buf) { \
		llvm_gcroot(self, 0);																								\
	  buf->write(pre);																										\
	  for(int i=0; i<self->size; i++) {																		\
	    if(i)																															\
				buf->write(sep);																								\
			buf->printer(self->elements[i]);																	\
		}																																		\
		buf->write(post);																										\
	}

ON_ARRAY_CLASSES(DEFINE_ARRAY_PRINT)

#undef DEFINE_ARRAY_PRINT
