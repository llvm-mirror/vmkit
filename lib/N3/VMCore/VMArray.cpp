//===----------------- VMArray.cpp - VM arrays ------------------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>

#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"


using namespace n3;

const sint32 VMArray::MaxArraySize = 268435455;

#define ACONS(name, elmt, size)                                             \
  name *name::acons(sint32 n, VMClassArray* atype) {                        \
    if (n < 0)                                                              \
      VMThread::get()->vm->negativeArraySizeException(n);                   \
    else if (n > VMArray::MaxArraySize)                                     \
      VMThread::get()->vm->outOfMemoryError(n);                             \
    name* res = (name*)                                                     \
      gc::operator new(sizeof(VMObject) + sizeof(sint32) + n * size,        \
                              VMObject::VT);                                \
    res->initialise(atype, n);                                              \
    res->setVirtualTable(name::VT);                                         \
    return res;                                                             \
  }

#define INITIALISE(name)                                                    \
  void name::initialise(VMCommonClass* atype, sint32 n) {                   \
    VMObject::initialise(atype);                                            \
    this->size = n;                                                         \
    for (int i = 0; i < n; i++)                                             \
      elements[i] = 0;                                                      \
  }                                                                         \

#define AT(name, elmt)                                                      \
  elmt name::at(sint32 offset) const {                                      \
    if (offset >= size)                                                     \
      VMThread::get()->vm->indexOutOfBounds(this, offset);                  \
    return elements[offset];                                                \
  }                                                                         \
  void name::setAt(sint32 offset, elmt value) {                             \
    if (offset >= size)                                                     \
      VMThread::get()->vm->indexOutOfBounds(this, offset);                  \
    elements[offset] = value;                                               \
  }

#define ARRAYCLASS(name, elmt, size)                                        \
  ACONS(name, elmt, size)                                                   \
  INITIALISE(name)                                                          \
  AT(name, elmt)                                                            \

ARRAYCLASS(ArrayUInt8,  uint8, 1)
ARRAYCLASS(ArraySInt8,  sint8, 1)
ARRAYCLASS(ArrayUInt16, uint16, 2)
ARRAYCLASS(ArraySInt16, sint16, 2)
ARRAYCLASS(ArrayUInt32, uint32, 4)
ARRAYCLASS(ArraySInt32, sint32, 4)
ARRAYCLASS(ArrayLong,   sint64, 8)
ARRAYCLASS(ArrayFloat,  float, 4)
ARRAYCLASS(ArrayDouble, double, 8)
ARRAYCLASS(ArrayObject, VMObject*, 4)

void VMArray::print(mvm::PrintBuffer *buf) const {
  assert(0 && "should not be here");
}

  
void ArrayUInt8::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArraySInt8::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayUInt16::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArraySInt16::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayUInt32::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArraySInt32::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS4(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayLong::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeS8(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayFloat::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeFP(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayDouble::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeFP(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}

void ArrayObject::print(mvm::PrintBuffer *buf) const {
  buf->write("Array<");
  for (int i = 0; i < size; i++) {
    buf->writeObj(elements[i]);
    buf->write(" ");
  }
  buf->write(">");
}


AT(UTF8, uint16)
INITIALISE(UTF8)

#undef AT
#undef INITIALISE
#undef ACONS
#undef ARRAYCLASS

UTF8* UTF8::acons(sint32 n, VMClassArray* atype) {
  if (n < 0)
    VMThread::get()->vm->negativeArraySizeException(n);
  else if (n > VMArray::MaxArraySize)
    VMThread::get()->vm->outOfMemoryError(n);
  uint32 size = sizeof(VMObject) + sizeof(sint32) + n * sizeof(uint16);
  UTF8* res = (UTF8*)gc::operator new(size, UTF8::VT);
  res->initialise(atype, n);
  return res;
}

void UTF8::print(mvm::PrintBuffer* buf) const {
  for (int i = 0; i < size; i++)
    buf->writeChar((char)elements[i]);
}

const UTF8* UTF8::extract(VirtualMachine *vm, uint32 start, uint32 end) const {
  uint32 len = end - start;
  uint16* buf = (uint16*)alloca(sizeof(uint16) * len);

  for (uint32 i = 0; i < len; i++) {
    buf[i] = at(i + start);
  }

  return readerConstruct(vm, buf, len);
}

const UTF8* UTF8::asciizConstruct(VirtualMachine* vm, const char* asciiz) {
  return vm->asciizConstructUTF8(asciiz);
}

const UTF8* UTF8::readerConstruct(VirtualMachine* vm, uint16* buf, uint32 n) {
  return vm->readerConstructUTF8(buf, n);
}

char* UTF8::UTF8ToAsciiz() const {
  mvm::NativeString* buf = mvm::NativeString::alloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf->setAt(i, elements[i]);
  }
  buf->setAt(size, 0);
  return buf->cString();
}

bool UTF8::equals(const UTF8 *string) const
{
  if (size != string->size) {
    return false;
  }
  
  for (sint32 i = 0; i < size; ++i) {
    if (at(i) != string->at(i)) {
      return false;
    }
  }
  
  return true;
}



