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

class VMArray : public VMObject {
public:
  static VirtualTable* VT;
  sint32 size;
  void* elements[1];
  static const sint32 MaxArraySize;
  static const llvm::Type* llvmType;

  static llvm::Constant* sizeOffset();
  static llvm::Constant* elementsOffset();
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

};

typedef VMArray* (*arrayCtor_t)(uint32 len, VMCommonClass* cl);

#define ARRAYCLASS(name, elmt)                                        \
class name : public VMObject {                                        \
public:                                                               \
  static VirtualTable* VT;                                            \
  static const llvm::Type* llvmType;                                  \
  sint32 size;                                                        \
  elmt elements[1];                                                   \
  virtual void print(mvm::PrintBuffer* buf) const;                    \
  virtual void TRACER;                                     \
}

ARRAYCLASS(ArrayUInt8,  uint8);
ARRAYCLASS(ArraySInt8,  sint8);
ARRAYCLASS(ArrayChar,   uint16);
ARRAYCLASS(ArrayUInt16, uint16);
ARRAYCLASS(ArraySInt16, sint16);
ARRAYCLASS(ArrayUInt32, uint32);
ARRAYCLASS(ArraySInt32, sint32);
ARRAYCLASS(ArrayLong,   sint64);
ARRAYCLASS(ArrayFloat,  float);
ARRAYCLASS(ArrayDouble, double);
ARRAYCLASS(ArrayObject, VMObject*);

#undef ARRAYCLASS


} // end namespace n3

#endif
