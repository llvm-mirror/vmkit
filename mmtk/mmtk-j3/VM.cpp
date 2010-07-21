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

#include "debug.h"

using namespace j3;

extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Extent_2 () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Address_2 () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__F () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__I () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Ljava_lang_String_2 () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__ () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__Ljava_lang_String_2 () { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_runtime_VM__1assert__ZLjava_lang_String_2 () {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysExit__I () {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysFail__Ljava_lang_String_2 () {
  // Just call abort because gcmalloc calls this function. If it were to
  // call printf, MMTkInline.inc could not be JIT-compiled.
  abort();
}

extern "C" void Java_org_j3_runtime_VM__1assert__Z (bool cond) {
  ASSERT(cond);
}

extern "C" bool Java_org_j3_runtime_VM_buildFor64Addr__ () { 
#if (__WORDSIZE==64)
  return true;
#else
  return false;
#endif
}

extern "C" bool Java_org_j3_runtime_VM_buildForIA32__ () { 
#if defined(__i386__) || defined(i386) || defined(_M_IX86)
  return true;
#else
  return false;
#endif
}

extern "C" bool Java_org_j3_runtime_VM_verifyAssertions__ () {
#ifdef DEBUG
  return true;
#else
  return false;
#endif
}
