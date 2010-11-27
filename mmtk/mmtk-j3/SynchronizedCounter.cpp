//===-------- SynchronizedCounter.cpp -------------------------------------===//
//===-------- Implementation of the SynchronizedCounter class  ------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

extern "C" void Java_org_j3_mmtk_SynchronizedCounter_reset__ (JavaObject* self) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_mmtk_SynchronizedCounter_increment__ (JavaObject* self) { UNIMPLEMENTED(); }

