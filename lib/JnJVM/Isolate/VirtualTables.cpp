//===--- VirtualTables.cpp - Virtual methods for Isolate objects ----------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SharedMaps.h"
#include "IsolateSharedLoader.h"

using namespace jnjvm;

#define INIT(X) VirtualTable* X::VT = 0

  INIT(JnjvmSharedLoader);
  INIT(SharedClassByteMap);
  INIT(UserClass);
  INIT(UserClassArray);
  INIT(UserConstantPool);

#undef INIT

void UserClass::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
  staticInstance->MARK_AND_TRACE;
  ctpInfo->MARK_AND_TRACE;
}

void UserClassPrimitive::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void UserClassArray::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void SharedClassByteMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->first->MARK_AND_TRACE;
  }
}

void JnjvmSharedLoader::TRACER {
  byteClasses->MARK_AND_TRACE;
}
