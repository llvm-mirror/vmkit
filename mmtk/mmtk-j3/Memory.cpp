//===------------ Memory.cpp - Implementation of the Memory class  --------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/mman.h>


#include "JavaObject.h"
#include "JavaThread.h"

using namespace jnjvm;
extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapStartConstant__ () {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getHeapEndConstant__ () {
  return (uintptr_t)0x80000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableStartConstant__ () {
  return (uintptr_t)0x30000000;
}

extern "C" uintptr_t Java_org_j3_mmtk_Memory_getAvailableEndConstant__ () {
  return (uintptr_t)0x80000000;
}

extern "C" sint32
Java_org_j3_mmtk_Memory_dzmmap__Lorg_vmmagic_unboxed_Address_2I(JavaObject* M,
                                                                void* start,
                                                                sint32 size) {
#if defined (__MACH__)
  uint32 flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
#else
  uint32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
  void* baseAddr = mmap((void*)start, size, PROT_READ | PROT_WRITE, flags,
                        -1, 0);
  if (baseAddr == MAP_FAILED) {
    perror("mmap");
    JavaThread::get()->printBacktrace(); abort();
  }

  return 0;
}

extern "C" void
Java_org_j3_mmtk_Memory_mprotect__Lorg_vmmagic_unboxed_Address_2I () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void
Java_org_j3_mmtk_Memory_munprotect__Lorg_vmmagic_unboxed_Address_2I () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void
Java_org_j3_mmtk_Memory_zero__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_Extent_2(JavaObject* M,
                                                                                          void* addr,
                                                                                          uintptr_t len) {
  memset(addr, 0, len);
}

extern "C" void
Java_org_j3_mmtk_Memory_zeroPages__Lorg_vmmagic_unboxed_Address_2I () {
  JavaThread::get()->printBacktrace(); abort();
}

extern "C" void
Java_org_j3_mmtk_Memory_dumpMemory__Lorg_vmmagic_unboxed_Address_2II () {
  JavaThread::get()->printBacktrace(); abort();
}
