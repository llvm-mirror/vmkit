//===----------------- JavaArray.cpp - Java arrays ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdarg>
#include <cstdlib>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "LockedMap.h"


using namespace jnjvm;

/// This value is the same value than IBM's JVM.
const sint32 JavaArray::MaxArraySize = 268435455;

/// The JVM defines constants for referencing arrays of primitive types.
const unsigned int JavaArray::T_BOOLEAN = 4;
const unsigned int JavaArray::T_CHAR = 5;
const unsigned int JavaArray::T_FLOAT = 6;
const unsigned int JavaArray::T_DOUBLE = 7;
const unsigned int JavaArray::T_BYTE = 8;
const unsigned int JavaArray::T_SHORT = 9;
const unsigned int JavaArray::T_INT = 10;
const unsigned int JavaArray::T_LONG = 11;

void UTF8::printUTF8(mvm::PrintBuffer* buf) const {
  for (int i = 0; i < size; i++)
    buf->writeChar((char)elements[i]);
}

void UTF8::print(mvm::PrintBuffer* buf) const {
  return printUTF8(buf);
}

const UTF8* UTF8::javaToInternal(Jnjvm* vm, unsigned int start,
                                 unsigned int len) const {
  UTF8* array = (UTF8*)vm->upcalls->ArrayOfChar->doNew(len, vm);
  uint16* java = array->elements;
  for (uint32 i = 0; i < len; i++) {
    uint16 cur = elements[start + i];
    if (cur == '.') java[i] = '/';
    else java[i] = cur;
  }

  return (const UTF8*)array;
}

// We also define a checked java to internal function to disallow
// users to load classes with '/'.
const UTF8* UTF8::checkedJavaToInternal(Jnjvm* vm, unsigned int start,
                                        unsigned int len) const {
  UTF8* array = (UTF8*)vm->upcalls->ArrayOfChar->doNew(len, vm);
  uint16* java = array->elements;
  for (uint32 i = 0; i < len; i++) {
    uint16 cur = elements[start + i];
    if (cur == '.') java[i] = '/';
    else if (cur == '/') return 0;
    else java[i] = cur;
  }

  return (const UTF8*)array;
}

const UTF8* UTF8::internalToJava(Jnjvm* vm, unsigned int start,
                                 unsigned int len) const {
  UTF8* array = (UTF8*)vm->upcalls->ArrayOfChar->doNew(len, vm);
  uint16* java = array->elements;
  for (uint32 i = 0; i < len; i++) {
    uint16 cur = elements[start + i];
    if (cur == '/') java[i] = '.';
    else java[i] = cur;
  }

  return (const UTF8*)array;
}

const UTF8* UTF8::extract(UTF8Map* map, uint32 start, uint32 end) const {
  uint32 len = end - start;
  uint16* buf = (uint16*)alloca(sizeof(uint16) * len);

  for (uint32 i = 0; i < len; i++) {
    buf[i] = elements[i + start];
  }

  return map->lookupOrCreateReader(buf, len);
}

const UTF8* UTF8::extract(Jnjvm* vm, uint32 start, uint32 end) const {
  uint32 len = end - start;
  UTF8* array = (UTF8*) vm->upcalls->ArrayOfChar->doNew(len, vm);
  uint16* buf = array->elements;

  for (uint32 i = 0; i < len; i++) {
    buf[i] = elements[i + start];
  }

  return (const UTF8*)array;
}

char* UTF8::UTF8ToAsciiz() const {
#ifndef DEBUG
  mvm::NativeString* buf = mvm::NativeString::alloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf->setAt(i, elements[i]);
  }
  buf->setAt(size, 0);
  return buf->cString();
#else
  // To bypass GC-allocation, use malloc here. Only when debugging.
  char* buf = (char*)malloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf[i] =  elements[i];
  }
  buf[size] = 0;
  return buf;
#endif
}

const UTF8* UTF8::acons(sint32 n, UserClassArray* cl,
                        mvm::BumpPtrAllocator& allocator) {
  assert(n >= 0 && "Creating an UTF8 with a size < 0");
  assert(n <= JavaArray::MaxArraySize && 
         "Creating an UTF8 with a size too big");

  UTF8* res = new (allocator, n) UTF8();
  res->initialise(cl);
  res->size = n; 
  return (const UTF8*)res;
}
