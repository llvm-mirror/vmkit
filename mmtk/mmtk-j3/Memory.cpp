//===------------ Memory.cpp - Implementation of the Memory class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"
#include "mvm/VirtualMachine.h"
#include "MMTkObject.h"

#include <sys/mman.h>

namespace mmtk {

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapStartConstant__ (MMTkObject* M) {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapEndConstant__ (MMTkObject* M) {
  return (uintptr_t)0x60000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableStartConstant__ (MMTkObject* M) {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableEndConstant__ (MMTkObject* M) {
  return (uintptr_t)0x60000000;
}

extern "C" sint32
Java_org_j3_mmtk_Memory_dzmmap__Lorg_vmmagic_unboxed_Address_2I(MMTkObject* M,
                                                                void* start,
                                                                sint32 size) {
  // Already mmapped during initialization.
  return 0;
}

extern "C" uint8_t
Java_org_j3_mmtk_Memory_mprotect__Lorg_vmmagic_unboxed_Address_2I (MMTkObject* M, uintptr_t address, sint32 size) {
  int val = mprotect((void*)address, size, PROT_NONE);
  return (val == 0);
}

extern "C" uint8_t
Java_org_j3_mmtk_Memory_munprotect__Lorg_vmmagic_unboxed_Address_2I (MMTkObject* M, uintptr_t address, sint32 size) {
  int val = mprotect((void*)address, size, PROT_READ | PROT_WRITE);
  return (val == 0);
}

extern "C" void
Java_org_j3_mmtk_Memory_zero__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_Extent_2(MMTkObject* M,
                                                                                          void* addr,
                                                                                          uintptr_t len) {
  memset(addr, 0, len);
}

extern "C" void
Java_org_j3_mmtk_Memory_zeroPages__Lorg_vmmagic_unboxed_Address_2I (MMTkObject* M, uintptr_t address, sint32 size) {
  UNIMPLEMENTED();
}

extern "C" void
Java_org_j3_mmtk_Memory_dumpMemory__Lorg_vmmagic_unboxed_Address_2II (MMTkObject* M, uintptr_t address, sint32 before, sint32 after) {
  UNIMPLEMENTED();
}

}
