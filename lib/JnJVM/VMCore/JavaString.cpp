//===-- JavaString.cpp - Internal correspondance with Java Strings --------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "LockedMap.h"

using namespace jnjvm;

JavaVirtualTable* JavaString::internStringVT = 0;

JavaString* JavaString::stringDup(const ArrayUInt16*& array, Jnjvm* vm) {
  UserClass* cl = vm->upcalls->newString;
  JavaString* res = (JavaString*)cl->doNew(vm);
  
  // It's a hashed string, set the destructor so that the string
  // removes itself from the vm string map. Do this ony if
  // internStringVT exists (in case of AOT).
  if (internStringVT) res->setVirtualTable(internStringVT);

  // No need to call the Java function: both the Java function and
  // this function do the same thing.
  res->value = array;
  res->count = array->size;
  res->offset = 0;
  res->cachedHashCode = 0;
  return res;
}

char* JavaString::strToAsciiz() {
  char* buf = new char[count + 1]; 
  for (sint32 i = 0; i < count; ++i) {
    buf[i] = value->elements[i + offset];
  }
  buf[count] =  0; 
  return buf;
}

const ArrayUInt16* JavaString::strToArray(Jnjvm* vm) {
  assert(value && "String without an array?");
  if (offset || (count != value->size)) {
    ArrayUInt16* array = 
      (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(count, vm);
    uint16* buf = array->elements;

    for (sint32 i = 0; i < count; i++) {
      buf[i] = value->elements[i + offset];
    }
    return array;
  } else {
    return value;
  }
}

void JavaString::stringDestructor(JavaString* str) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  assert(vm && "No vm when destroying a string");
  if (str->value) vm->hashStr.remove(str->value, str);
}

JavaString* JavaString::internalToJava(const UTF8* name, Jnjvm* vm) {
  ArrayUInt16* array = 
    (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(name->size, vm);
  
  uint16* java = array->elements;
  for (sint32 i = 0; i < name->size; i++) {
    uint16 cur = name->elements[i];
    if (cur == '/') java[i] = '.';
    else java[i] = cur;
  }

  return vm->constructString(array);
}

const UTF8* JavaString::javaToInternal(UTF8Map* map) const {
  uint16* java = (uint16*)alloca(sizeof(uint16) * count);

  for (sint32 i = 0; i < count; ++i) {
    uint16 cur = value->elements[offset + i];
    if (cur == '.') java[i] = '/';
    else java[i] = cur;
  }
  
  return map->lookupOrCreateReader(java, count);
}
