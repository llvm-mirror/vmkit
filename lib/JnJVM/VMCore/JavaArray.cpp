//===----------------- JavaArray.cpp - Java arrays ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdarg.h>
#include <stdlib.h>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "JavaThread.h"


using namespace jnjvm;

const sint32 JavaArray::MaxArraySize = 268435455;
const unsigned int JavaArray::T_BOOLEAN = 4;
const unsigned int JavaArray::T_CHAR = 5;
const unsigned int JavaArray::T_FLOAT = 6;
const unsigned int JavaArray::T_DOUBLE = 7;
const unsigned int JavaArray::T_BYTE = 8;
const unsigned int JavaArray::T_SHORT = 9;
const unsigned int JavaArray::T_INT = 10;
const unsigned int JavaArray::T_LONG = 11;

ClassArray* JavaArray::ofByte = 0;
ClassArray* JavaArray::ofChar = 0;
ClassArray* JavaArray::ofInt = 0;
ClassArray* JavaArray::ofShort = 0;
ClassArray* JavaArray::ofBool = 0;
ClassArray* JavaArray::ofLong = 0;
ClassArray* JavaArray::ofFloat = 0;
ClassArray* JavaArray::ofDouble = 0;
ClassArray* JavaArray::ofString = 0;
ClassArray* JavaArray::ofObject = 0;

// This will force linking runtime methods
extern "C" void negativeArraySizeException(sint32 val);
extern "C" void outOfMemoryError(sint32 val);

#ifndef MULTIPLE_VM
#define ACONS(name, elmt, primSize)                                         \
  name *name::acons(sint32 n, ClassArray* atype, Jnjvm* vm) {               \
    if (n < 0)                                                              \
      negativeArraySizeException(n);                                        \
    else if (n > JavaArray::MaxArraySize)                                   \
      outOfMemoryError(n);                                                  \
    name* res = (name*)                                                     \
      (Object*) operator new(sizeof(name) + n * primSize, name::VT);        \
    res->initialise(atype);                                                 \
    res->size = n;                                                          \
    return res;                                                             \
  }
#else
#define ACONS(name, elmt, primSize)                                         \
  name *name::acons(sint32 n, ClassArray* atype, Jnjvm* vm) {               \
    if (n < 0)                                                              \
      negativeArraySizeException(n);                                        \
    else if (n > JavaArray::MaxArraySize)                                   \
      outOfMemoryError(n);                                                  \
    name* res = (name*)                                                     \
      (Object*) vm->allocateObject(sizeof(name) + n * primSize, name::VT);  \
    res->initialise(atype);                                                 \
    res->size = n;                                                          \
    return res;                                                             \
  }
#endif

#define AT(name, elmt)                                                      \
  elmt name::at(sint32 offset) const {                                      \
    if (offset >= size)                                                     \
      JavaThread::get()->isolate->indexOutOfBounds(this, offset);           \
    return elements[offset];                                                \
  }                                                                         \
  void name::setAt(sint32 offset, elmt value) {                             \
    if (offset >= size)                                                     \
      JavaThread::get()->isolate->indexOutOfBounds(this, offset);           \
    elements[offset] = value;                                               \
  }

#define ARRAYCLASS(name, elmt, size)                                        \
  ACONS(name, elmt, size)                                                   \
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
ARRAYCLASS(ArrayObject, JavaObject*, sizeof(JavaObject*))

#undef ARRAYCLASS
#undef ACONS
#undef AT

void JavaArray::print(mvm::PrintBuffer *buf) const {
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


void ArrayUInt16::print(mvm::PrintBuffer* buf) const {
  for (int i = 0; i < size; i++)
    buf->writeChar((char)elements[i]);
}

const UTF8* UTF8::javaToInternal(Jnjvm* vm, unsigned int start,
                                 unsigned int len) const {
  uint16* java = (uint16*) alloca(len * sizeof(uint16));
  for (uint32 i = 0; i < len; i++) {
    uint16 cur = elements[start + i];
    if (cur == '.') java[i] = '/';
    else java[i] = cur;
  }

  return readerConstruct(vm, java, len);
}

const UTF8* UTF8::internalToJava(Jnjvm *vm, unsigned int start,
                                 unsigned int len) const {
  uint16* java = (uint16*) alloca(len * sizeof(uint16));
  for (uint32 i = 0; i < len; i++) {
    uint16 cur = elements[start + i];
    if (cur == '/') java[i] = '.';
    else java[i] = cur;
  }

  return readerConstruct(vm, java, len);
}

const UTF8* UTF8::extract(Jnjvm *vm, uint32 start, uint32 end) const {
  uint32 len = end - start;
  uint16* buf = (uint16*)alloca(sizeof(uint16) * len);

  for (uint32 i = 0; i < len; i++) {
    buf[i] = elements[i + start];
  }

  return readerConstruct(vm, buf, len);
}

const UTF8* UTF8::asciizConstruct(Jnjvm* vm, char* asciiz) {
  return vm->asciizConstructUTF8(asciiz);
}

const UTF8* UTF8::readerConstruct(Jnjvm* vm, uint16* buf, uint32 n) {
  return vm->readerConstructUTF8(buf, n);
}

char* UTF8::UTF8ToAsciiz() const {
  /*mvm::NativeString* buf = mvm::NativeString::alloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf->setAt(i, elements[i]);
  }
  buf->setAt(size, 0);
  return buf->cString();*/
  char* buf = (char*)malloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf[i] =  elements[i];
  }
  buf[size] = 0;
  return buf;
}
