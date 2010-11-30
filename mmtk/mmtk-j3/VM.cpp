//===-------------- VM.cpp - Implementation of the VM class  --------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"
#include "MMTkObject.h"

namespace mmtk {

extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Extent_2 (uintptr_t e) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Lorg_vmmagic_unboxed_Address_2 (uintptr_t a) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__F (float f) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__I (int i) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWrite__Ljava_lang_String_2 (MMTkString* msg) { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__ () { UNIMPLEMENTED(); }
extern "C" void Java_org_j3_runtime_VM_sysWriteln__Ljava_lang_String_2 (MMTkString* msg) { UNIMPLEMENTED(); }

extern "C" void Java_org_j3_runtime_VM__1assert__ZLjava_lang_String_2 (bool cond, MMTkString* msg) {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysExit__I (int i) {
  ABORT();
}

extern "C" void Java_org_j3_runtime_VM_sysFail__Ljava_lang_String_2 (MMTkString* msg) {
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
  // Note that DEBUG is defined in make ENABLE_OPTIMIZED=1.
  // You must provide DISABLE_ASSERTIONS=1 to not have DEBUG defined.
  // To generate MMTkInline.inc, this function returns false.
#if 0
  return true;
#else
  return false;
#endif
}

} // namespace mmtk
