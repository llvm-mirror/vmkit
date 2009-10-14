//===-------------------- VMArray.h - VM arrays ---------------------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_VM_ARRAY_H
#define N3_VM_ARRAY_H

#include "mvm/PrintBuffer.h"

#include "llvm/Constants.h"
#include "llvm/Type.h"

#include "types.h"

#include "VMObject.h"

#include "UTF8.h"

namespace n3 {

class VMClassArray;
class VMCommonClass;
class VMObject;

	// never allocate a VMArray, it is just a C++ type to access N3 object
class VMArray : public VMObject {
	void *operator new(size_t n) { return VMObject::operator new(n, 0); }

public:
  sint32 size;
  void* elements[1];

  static const sint32 MaxArraySize;
  static const llvm::Type* llvmType;
  static llvm::Constant* sizeOffset();
  static llvm::Constant* elementsOffset();
};

#define ON_ARRAY_PRIMITIVE_CLASSES(_)													\
	_(UInt8,  uint8,     1, writeS4,   "Array<", " ", ">") \
	_(SInt8,  sint8,     1, writeS4,   "Array<", " ", ">") \
	_(Char,   uint16,    2, writeChar, "",       "",  "")  \
	_(UInt16, uint16,    2, writeS4,   "Array<", " ", ">") \
	_(SInt16, sint16,    2, writeS4,   "Array<", " ", ">") \
	_(UInt32, uint32,    4, writeS4,   "Array<", " ", ">") \
	_(SInt32, sint32,    4, writeS4,   "Array<", " ", ">") \
	_(UInt64, uint64,    8, writeS8,   "Array<", " ", ">") \
	_(SInt64, sint64,    8, writeS8,   "Array<", " ", ">") \
	_(Float,  float,     4, writeFP,   "Array<", " ", ">") \
	_(Double, double,    8, writeFP,   "Array<", " ", ">")

#define ON_ARRAY_CLASSES(_)																		\
	ON_ARRAY_PRIMITIVE_CLASSES(_)																\
	_(Object, VMObject*, 4, writeObj,  "Array<", " ", ">")


	// never allocate a VMArray, it is just a C++ type to access N3 object
#define DEFINE_ARRAY_CLASS(name, elmt, nbb, printer, pre, sep, post)		\
	class Array##name : public VMObject {																	\
		void *operator new(size_t n) { return VMObject::operator new(n, 0); } \
	public:																																\
		static const llvm::Type* llvmType;                                  \
		sint32 size;                                                        \
		elmt elements[1];                                                   \
		static void do_print(const Array##name *self, mvm::PrintBuffer* buf); \
	};

ON_ARRAY_CLASSES(DEFINE_ARRAY_CLASS)

#undef DEFINE_ARRAY_CLASS


} // end namespace n3

#endif
