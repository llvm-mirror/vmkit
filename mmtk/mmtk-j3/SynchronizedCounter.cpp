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

using namespace jnjvm;

extern "C" void Java_org_j3_mmtk_SynchronizedCounter_reset__ () { abort(); }
extern "C" void Java_org_j3_mmtk_SynchronizedCounter_increment__ () { abort(); }

