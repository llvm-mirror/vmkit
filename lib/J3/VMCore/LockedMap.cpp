//===------- LockedMap.cpp - Implementation of the UTF8 map ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <map>

#include "JavaArray.h"
#include "JavaString.h"
#include "LockedMap.h"

using namespace j3;

void StringMap::insert(JavaString* str) {
  const ArrayUInt16* array = NULL;
  llvm_gcroot(str, 0);
  llvm_gcroot(array, 0);
  array = JavaString::getValue(str);
  StringMap::iterator it = map.insert(std::make_pair(array, str)).first;
  assert(map.find(array)->second == str);
  assert(map.find(array)->first == array);
  assert(&(map.find(array)->second) == &(it->second));
  assert(&(map.find(array)->first) == &(it->first));
}
