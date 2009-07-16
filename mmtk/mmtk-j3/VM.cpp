//===-------------- VM.cpp - Implementation of the VM class  --------------===//
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

extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Extent_2 () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Address_2 () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__F () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__I () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Ljava_lang_String_2 () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__ () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__Ljava_lang_String_2 () { abort(); }
extern "C" void Java_org_j3_runtime_VM__1assert__ZLjava_lang_String_2 () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysExit__I () { abort(); }
extern "C" void Java_org_j3_runtime_VM_sysFail__Ljava_lang_String_2 () { abort(); }

extern "C" void Java_org_j3_runtime_VM__1assert__Z (bool cond) {
  
  assert(cond);
}

extern "C" bool Java_org_j3_runtime_VM_buildFor64Addr__ () { 
  return false;
}

extern "C" bool Java_org_j3_runtime_VM_buildForIA32__ () { 
  return true;
}

extern "C" bool Java_org_j3_runtime_VM_verifyAssertions__ () {
  return true;
}

