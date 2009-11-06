//===-------- Selected.cpp - Implementation of the Selected class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "MutatorThread.h"

using namespace jnjvm;

extern "C" JavaObject* Java_org_j3_config_Selected_00024Mutator_get__() {
  return (JavaObject*)mvm::MutatorThread::get()->MutatorContext;
}
