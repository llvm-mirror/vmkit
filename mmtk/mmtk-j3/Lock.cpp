//===----------- Lock.cpp - Implementation of the Lock class  -------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "mvm/Threads/Locks.h"
#include "mvm/VirtualMachine.h"

#include "debug.h"

using namespace j3;

struct Lock {
  JavaObject base;
  mvm::SpinLock spin;
  JavaString* name;
};


extern "C" void Java_org_j3_mmtk_Lock_acquire__(Lock* l) {
  bool saved = mvm::Thread::get()->inRV;
  mvm::Thread::get()->inRV = true;
  l->spin.acquire();
  mvm::Thread::get()->inRV = saved;
}
extern "C" void Java_org_j3_mmtk_Lock_check__I (Lock* l, int i) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_mmtk_Lock_release__(Lock* l) {
  bool saved = mvm::Thread::get()->inRV;
  mvm::Thread::get()->inRV = true;
  l->spin.release();
  mvm::Thread::get()->inRV = saved;
}
