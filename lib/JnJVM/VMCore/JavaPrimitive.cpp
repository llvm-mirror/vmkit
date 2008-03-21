//===--- JavaPrimitive.cpp - Native functions for primitive values --------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include <string.h>

#include "JavaClass.h"
#include "JavaPrimitive.h"
#include "JavaTypes.h"

using namespace jnjvm;

void JavaPrimitive::print(mvm::PrintBuffer* buf) {
  buf->write("Primitive");
}

JavaPrimitive* JavaPrimitive::byteIdToPrimitive(char id) {
  for (uint32 i = 0; i < JavaPrimitive::primitives.size(); ++i) {
    JavaPrimitive* cur = JavaPrimitive::primitives[i];
    if (cur->funcs->byteId == id) return cur;
  }
  return 0;
}

JavaPrimitive* JavaPrimitive::asciizToPrimitive(char* asciiz) {
  for (uint32 i = 0; i < JavaPrimitive::primitives.size(); ++i) {
    JavaPrimitive* cur = JavaPrimitive::primitives[i];
    if (!(strcmp(asciiz, cur->funcs->asciizName))) return cur;
  }
  return 0;
}

JavaPrimitive* JavaPrimitive::bogusClassToPrimitive(CommonClass* cl) {
  for (uint32 i = 0; i < JavaPrimitive::primitives.size(); ++i) {
    JavaPrimitive* cur = JavaPrimitive::primitives[i];
    if (cur->classType == cl) return cur;
  }
  return 0;
}

JavaPrimitive* JavaPrimitive::classToPrimitive(CommonClass* cl) {
  for (uint32 i = 0; i < JavaPrimitive::primitives.size(); ++i) {
    JavaPrimitive* cur = JavaPrimitive::primitives[i];
    if (cur->className == cl->name) return cur;
  }
  return 0;
}

JavaPrimitive* JavaPrimitive::funcsToPrimitive(AssessorDesc* func) {
  for (uint32 i = 0; i < JavaPrimitive::primitives.size(); ++i) {
    JavaPrimitive* cur = JavaPrimitive::primitives[i];
    if (cur->funcs == func) return cur;
  }
  return 0;
}
