//===-- RuntimeHelpers.cpp - Implement rt.jar functions needed by MMTk  --===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MMTkObject.h"
#include "JavaObject.h"
#include "JavaClass.h"

namespace mmtk {

extern "C" uint16_t MMTkCharAt(MMTkString* str, uint32_t index) {
  return str->value->elements[index];
}

extern "C" j3::JavaObject* MMTkGetClass(j3::JavaObject* obj) {
  return ((j3::JavaVirtualTable*)obj->getVirtualTable())->cl->delegatee[0];
}

}
