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


JavaString* JavaString::stringDup(const UTF8*& utf8, Jnjvm* vm) {
  UserClass* cl = vm->upcalls->newString;
  JavaString* res = (JavaString*)cl->doNew(vm);

  // No need to call the Java function: both the Java function and
  // this function do the same thing.
  res->value = utf8;
  res->count = utf8->size;
  res->offset = 0;
  res->cachedHashCode = 0;
  return res;
}

char* JavaString::strToAsciiz() {
  mvm::NativeString* buf = mvm::NativeString::alloc(count + 1); 
  for (sint32 i = 0; i < count; ++i) {
    buf->setAt(i, value->elements[i + offset]);
  }
  buf->setAt(count, 0); 
  return buf->cString();
}

const UTF8* JavaString::strToUTF8(Jnjvm* vm) {
  const UTF8* utf8 = this->value;
  if (offset || (offset + count <= utf8->size)) {
    return utf8->extract(vm, offset, offset + count);
  } else {
    return utf8;
  }
}

void JavaString::stringDestructor(JavaString* str) {
  Jnjvm* vm = JavaThread::get()->isolate;
  assert(vm && "No vm when destroying a string");
  if (str->value) vm->hashStr->remove(str->value, str);
}
