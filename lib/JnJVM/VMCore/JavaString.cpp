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
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace jnjvm;


JavaString* JavaString::stringDup(const UTF8*& utf8, Jnjvm* vm) {
  UserClass* cl = vm->upcalls->newString;
  JavaString* res = (JavaString*)malloc(cl->getVirtualSize());
#ifdef ISOLATE_SHARING
  /// Do this for now, but we will have to change it to duplicate the UTF8.
  /// UTF8 that dont have a class are shared UTF8.
  if (!utf8->classOf) ((UTF8*)utf8)->classOf = vm->upcalls->ArrayOfChar;
#endif
  ((void**)res)[0] = cl->getVirtualVT();
  res->classOf = cl;

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
    return utf8->extract(vm->hashUTF8, offset, offset + count);
  } else {
    return utf8;
  }
}
