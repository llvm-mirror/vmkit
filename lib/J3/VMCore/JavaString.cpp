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

JavaString* JavaString::stringDup(const ArrayUInt16*& _array, Jnjvm* vm) {
  
  JavaString* res = 0;
  const ArrayUInt16* array = 0;
  llvm_gcroot(array, 0);
  llvm_gcroot(res, 0);

  array = _array;
  UserClass* cl = vm->upcalls->newString;
  res = (JavaString*)cl->doNew(vm);
  
  // It's a hashed string, set the destructor so that the string
  // removes itself from the vm string map. Do this only if
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
  JavaString* self = this;
  ArrayUInt16* array = 0;
  llvm_gcroot(self, 0);
  llvm_gcroot(array, 0);

  assert(self->value && "String without an array?");
  if (self->offset || (self->count != self->value->size)) {
    array = (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(self->count, vm);
    uint16* buf = array->elements;

    for (sint32 i = 0; i < count; i++) {
      buf[i] = self->value->elements[i + self->offset];
    }
    return array;
  } else {
    return self->value;
  }
}

void JavaString::stringDestructor(JavaString* str) {
  llvm_gcroot(str, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  assert(vm && "No vm when destroying a string");
  if (str->value) vm->hashStr.removeUnlocked(str->value, str);
}

JavaString* JavaString::internalToJava(const UTF8* name, Jnjvm* vm) {
  
  ArrayUInt16* array = 0;
  llvm_gcroot(array, 0);

  array = (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(name->size, vm);
  
  uint16* java = array->elements;
  for (sint32 i = 0; i < name->size; i++) {
    uint16 cur = name->elements[i];
    if (cur == '/') java[i] = '.';
    else java[i] = cur;
  }

  return vm->constructString(array);
}

const UTF8* JavaString::javaToInternal(UTF8Map* map) const {
  const JavaString* self = this;
  llvm_gcroot(self, 0);
  
  uint16* java = (uint16*)alloca(sizeof(uint16) * self->count);

  for (sint32 i = 0; i < count; ++i) {
    uint16 cur = self->value->elements[offset + i];
    if (cur == '.') java[i] = '/';
    else java[i] = cur;
  }
  
  return map->lookupOrCreateReader(java, self->count);
}
