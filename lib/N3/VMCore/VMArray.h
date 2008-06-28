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

namespace n3 {

class VMClassArray;
class VMCommonClass;
class VMObject;
class VirtualMachine;

class VMArray : public VMObject {
public:
  static VirtualTable* VT;
  sint32 size;
  void* elements[1];
  static const sint32 MaxArraySize;
  static const llvm::Type* llvmType;

  static llvm::ConstantInt* sizeOffset();
  static llvm::ConstantInt* elementsOffset();
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
  static name *acons(sint32 n, VMClassArray* cl);                     \
  void initialise(VMCommonClass* atype, sint32 n);                    \
  elmt at(sint32) const;                                              \
  void setAt(sint32, elmt);                                           \
  virtual void print(mvm::PrintBuffer* buf) const;                    \
  virtual void TRACER;                                     \
}

ARRAYCLASS(ArrayUInt8,  uint8);
ARRAYCLASS(ArraySInt8,  sint8);
ARRAYCLASS(ArrayUInt16, uint16);
ARRAYCLASS(ArraySInt16, sint16);
ARRAYCLASS(ArrayUInt32, uint32);
ARRAYCLASS(ArraySInt32, sint32);
ARRAYCLASS(ArrayLong,   sint64);
ARRAYCLASS(ArrayFloat,  float);
ARRAYCLASS(ArrayDouble, double);

#undef ARRAYCLASS

class ArrayObject : public VMObject {
public:
  static VirtualTable* VT;
  static const llvm::Type* llvmType;
  sint32 size;
  VMObject* elements[1];
  static ArrayObject *acons(sint32 n, VMClassArray* cl);
  void initialise(VMCommonClass* atype, sint32 n);
  VMObject* at(sint32) const;
  void setAt(sint32, VMObject*);
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
};

class UTF8 : public VMObject {
public:
  static VirtualTable* VT;
  sint32 size;
  uint16 elements[1];

  static const llvm::Type* llvmType;
  static UTF8* acons(sint32 n, VMClassArray* cl);
  void initialise(VMCommonClass* atype, sint32 n);
  
  unsigned short int at(sint32) const;
  void setAt(sint32, uint16);
  
  virtual void print(mvm::PrintBuffer* buf) const;

  char* UTF8ToAsciiz() const;
  static const UTF8* asciizConstruct(VirtualMachine *vm, char* asciiz);
  static const UTF8* readerConstruct(VirtualMachine *vm, uint16* buf, uint32 n);

  const UTF8* extract(VirtualMachine *vm, uint32 start, uint32 len) const;
};

} // end namespace n3

#endif
