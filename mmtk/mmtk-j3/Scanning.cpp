//===-------- Scanning.cpp - Implementation of the Scanning class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaObject.h"
#include "JavaThread.h"

using namespace jnjvm;

extern "C" void Java_org_j3_mmtk_Scanning_computeThreadRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  mvm::Collector::TraceLocal = (uintptr_t)TL;
  mvm::Thread* th = mvm::Thread::get();
  th->inGC = true;
  th->MyVM->startCollection();
  th->MyVM->rendezvous.synchronize();
}

extern "C" void Java_org_j3_mmtk_Scanning_computeGlobalRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  
  assert(mvm::Collector::TraceLocal == (uintptr_t)TL && "Mismatch in trace local");
  
  mvm::Thread* th = mvm::Thread::get();
  mvm::Thread* tcur = th;

  // (1) Trace the VM.
  th->MyVM->tracer();

  // (2) Trace the threads.
  do {
    tcur->tracer();
    tcur = (mvm::Thread*)tcur->next();
  } while (tcur != th);
}

extern "C" void Java_org_j3_mmtk_Scanning_computeStaticRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  // Nothing to do.
}

extern "C" void Java_org_j3_mmtk_Scanning_resetThreadCounter__ (JavaObject* Scanning) {
  // Nothing to do.
}

extern "C" void Java_org_j3_mmtk_Scanning_specializedScanObject__ILorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* Scanning, uint32_t id, JavaObject* TC, JavaObject* obj) {
  assert(mvm::Collector::TraceLocal == (uintptr_t)TC && "Mismatch in trace local");
  obj->tracer();
}

extern "C" void Java_org_j3_mmtk_Scanning_scanObject__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_Scanning_precopyChildren__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2 () { JavaThread::get()->printBacktrace(); abort(); }
extern "C" void Java_org_j3_mmtk_Scanning_preCopyGCInstances__Lorg_mmtk_plan_TraceLocal_2 () { JavaThread::get()->printBacktrace(); abort(); }
