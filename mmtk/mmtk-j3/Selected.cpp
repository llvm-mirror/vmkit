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

using namespace jnjvm;

#if 1

extern "C" JavaObject* org_j3_config_Selected_4Mutator_static;
extern "C" JavaObject* org_j3_config_Selected_4Collector_static;


extern "C" JavaObject* Java_org_j3_config_Selected_00024Collector_get__() {
  JavaObject* obj = org_j3_config_Selected_4Collector_static;
  return obj;
}

extern "C" JavaObject* Java_org_j3_config_Selected_00024Mutator_get__() {
  JavaObject* obj = org_j3_config_Selected_4Mutator_static;
  return obj;
}


#else
extern "C" JavaObject* Java_org_j3_config_Selected_00024Collector_get__() {
  return 0;
}

extern "C" JavaObject* Java_org_j3_config_Selected_00024Mutator_get__() {
  return 0;
}
#endif
