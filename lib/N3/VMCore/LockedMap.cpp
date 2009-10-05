//===------- LockedMap.cpp - Implementation of the UTF8 map ---------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <map>

#include "Assembly.h"
#include "LockedMap.h"
#include "MSCorlib.h"
#include "N3.h"
#include "VMArray.h"
#include "VMClass.h"

#include <string.h>

using namespace n3;
