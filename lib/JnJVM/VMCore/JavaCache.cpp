//===------ JavaCache.cpp - Inline cache for virtual calls -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "types.h"

using namespace jnjvm;
