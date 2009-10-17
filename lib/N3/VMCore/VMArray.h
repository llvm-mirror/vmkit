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

	// never allocate a VMArray, it is just a C++ type to access N3 object
#define DEFINE_ARRAY_CLASS(name, type)																	\
	class Array##name : public VMObject {																	\
		void *operator new(size_t n) { return VMObject::operator new(n, 0); } \
	public:																																\
		static const llvm::Type* llvmType;                                  \
		sint32 size;                                                        \
		type elements[1];                                                   \
		static void do_print(const Array##name *self, mvm::PrintBuffer* buf); \
	};

ON_TYPES(DEFINE_ARRAY_CLASS, _F_NT)

#undef DEFINE_ARRAY_CLASS


} // end namespace n3

#endif
