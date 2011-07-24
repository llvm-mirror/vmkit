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

static const uintptr_t MemoryStart = 0x50000000;
static const uintptr_t MemorySize = 0x40000000;

class InitCollector {
public:
  InitCollector() {
#if defined (__MACH__)
    uint32 flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
#else
    uint32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
    void* baseAddr = mmap((void*)MemoryStart, MemorySize, PROT_READ | PROT_WRITE,
                          flags, -1, 0);
    if (baseAddr == MAP_FAILED) {
      perror("mmap");
      abort();
    }
  }
};

// Allocate the memory for MMTk right now, to avoid conflicts with other allocators.
InitCollector initCollector;

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapStartConstant__ (MMTkObject* M) {
  return MemoryStart;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapEndConstant__ (MMTkObject* M) {
  return MemoryStart + MemorySize;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableStartConstant__ (MMTkObject* M) {
  return Java_org_j3_mmtk_Memory_getHeapStartConstant__ (M);
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableEndConstant__ (MMTkObject* M) {
  return Java_org_j3_mmtk_Memory_getHeapEndConstant__ (M);
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
