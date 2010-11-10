//===-------------- VM.cpp - Implementation of the VM class  --------------===//
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

#include "debug.h"

using namespace j3;

extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Extent_2 (uintptr_t e) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Address_2 (uintptr_t a) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__F (float f) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__I (int i) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Ljava_lang_String_2 (JavaString* msg) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__ () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__Ljava_lang_String_2 (JavaString* msg) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_runtime_VM__1assert__ZLjava_lang_String_2 (bool cond, JavaString* msg) {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysExit__I (int i) {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysFail__Ljava_lang_String_2 (JavaString* msg) {
  // Just call abort because gcmalloc calls this function. If it were to
  // call printf, MMTkInline.inc could not be JIT-compiled.
  abort();
}

extern "C" void Java_org_j3_runtime_VM__1assert__Z (uint8_t cond) {
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
#if 1//def DEBUG
  return true;
#else
  return false;
#endif
}
