//===-------- Scanning.cpp - Implementation of the Scanning class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/VirtualMachine.h"

#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

extern "C" void Java_org_j3_mmtk_Scanning_computeThreadRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  // When entering this function, all threads are waiting on the rendezvous to
  // finish.
  mvm::Thread* th = mvm::Thread::get();
  mvm::StackScanner& sc = th->MyVM->scanner;
  mvm::Thread* tcur = th;
  
  do {
    sc.scanStack(tcur, reinterpret_cast<uintptr_t>(TL));
    tcur = (mvm::Thread*)tcur->next();
  } while (tcur != th);
}

extern "C" void Java_org_j3_mmtk_Scanning_computeGlobalRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) { 
  mvm::Thread::get()->MyVM->tracer(reinterpret_cast<uintptr_t>(TL));
  
	mvm::Thread* th = mvm::Thread::get();
  mvm::Thread* tcur = th;
  
  do {
    tcur->tracer(reinterpret_cast<uintptr_t>(TL));
    tcur = (mvm::Thread*)tcur->next();
  } while (tcur != th);

}

extern "C" void Java_org_j3_mmtk_Scanning_computeStaticRoots__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  // Nothing to do.
}

extern "C" void Java_org_j3_mmtk_Scanning_resetThreadCounter__ (JavaObject* Scanning) {
  // Nothing to do.
}

extern "C" void Java_org_j3_mmtk_Scanning_specializedScanObject__ILorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* Scanning, uint32_t id, JavaObject* TC, JavaObject* obj) ALWAYS_INLINE;

extern "C" void Java_org_j3_mmtk_Scanning_specializedScanObject__ILorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2 (JavaObject* Scanning, uint32_t id, JavaObject* TC, JavaObject* obj) {
  assert(obj && "No object to trace");
  assert(obj->getVirtualTable() && "No virtual table");
  assert(obj->getVirtualTable()->tracer && "No tracer in VT");
  obj->tracer(reinterpret_cast<uintptr_t>(TC));
}

extern "C" void Java_org_j3_mmtk_Scanning_preCopyGCInstances__Lorg_mmtk_plan_TraceLocal_2 (JavaObject* Scanning, JavaObject* TL) {
  // Nothing to do, there are no GC objects on which the GC depends.
}

extern "C" void Java_org_j3_mmtk_Scanning_scanObject__Lorg_mmtk_plan_TransitiveClosure_2Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* Scanning, uintptr_t TC, JavaObject* obj) {
  assert(obj && "No object to trace");
  assert(obj->getVirtualTable() && "No virtual table");
  assert(obj->getVirtualTable()->tracer && "No tracer in VT");
  obj->tracer(TC);
}

extern "C" void Java_org_j3_mmtk_Scanning_precopyChildren__Lorg_mmtk_plan_TraceLocal_2Lorg_vmmagic_unboxed_ObjectReference_2 (
    JavaObject* Scanning, JavaObject TL, uintptr_t ref) { UNIMPLEMENTED(); }
