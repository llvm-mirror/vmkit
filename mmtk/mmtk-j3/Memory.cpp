//===------------ Memory.cpp - Implementation of the Memory class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/mman.h>

#include "mvm/VirtualMachine.h"
#include "JavaObject.h"
#include "JavaThread.h"

#include "debug.h"

using namespace j3;

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapStartConstant__ () {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapEndConstant__ () {
  return (uintptr_t)0x70000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableStartConstant__ () {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableEndConstant__ () {
  return (uintptr_t)0x70000000;
}

extern "C" sint32
Java_org_j3_mmtk_Memory_dzmmap__Lorg_vmmagic_unboxed_Address_2I(JavaObject* M,
                                                                void* start,
                                                                sint32 size) {
  // Already mmapped during initialization.
  return 0;
}

extern "C" void
Java_org_j3_mmtk_Memory_mprotect__Lorg_vmmagic_unboxed_Address_2I () {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_Memory_munprotect__Lorg_vmmagic_unboxed_Address_2I () {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_Memory_zero__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_Extent_2(JavaObject* M,
                                                                                          void* addr,
                                                                                          uintptr_t len) {
  memset(addr, 0, len);
}

extern "C" void
Java_org_j3_mmtk_Memory_zeroPages__Lorg_vmmagic_unboxed_Address_2I () {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_Memory_dumpMemory__Lorg_vmmagic_unboxed_Address_2II () {
  UNIMPLEMENTED();
}
