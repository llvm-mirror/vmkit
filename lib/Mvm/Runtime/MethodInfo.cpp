//===------- MethodInfo.cpp - Runtime information for methods -------------===//
//
//                        The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Allocator.h"
#include "mvm/MethodInfo.h"
#include "MvmGC.h"

#include <dlfcn.h>

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif

using namespace mvm;

void CamlMethodInfo::scan(void* TL, void* ip, void* addr) {
  if (CF) {
    //uintptr_t spaddr = (uintptr_t)addr + CF->FrameSize + sizeof(void*);
    uintptr_t spaddr = ((uintptr_t*)addr)[0];
    for (uint16 i = 0; i < CF->NumLiveOffsets; ++i) {
      Collector::scanObject((void**)(spaddr + CF->LiveOffsets[i]));
    }
  }
}

void StaticCamlMethodInfo::print(void* ip, void* addr) {
  fprintf(stderr, "; %p in %s static method\n", ip, name);
}

void DefaultMethodInfo::print(void* ip, void* addr) {
  Dl_info info;
  int res = dladdr(ip, &info);
  if (res != 0) {
    fprintf(stderr, "; %p in %s\n",  ip, info.dli_sname);
  } else {
    fprintf(stderr, "; %p in Unknown method\n",  ip);
  }
}

DefaultMethodInfo DefaultMethodInfo::DM;

void DefaultMethodInfo::scan(void* TL, void* ip, void* addr) {
}


MethodInfo* StartFunctionMap::IPToMethodInfo(void* ip) {
  FunctionMapLock.acquire();
  std::map<void*, MethodInfo*>::iterator E = Functions.end();
  std::map<void*, MethodInfo*>::iterator I = Functions.find(ip);
  MethodInfo* MI = 0;
  if (I == E) {
    Dl_info info;
    int res = dladdr(ip, &info);
    if (res != 0) {
      I = Functions.find(info.dli_saddr);
      if (I == E) {
        // The method is static, and we have no information for it.
        // Just return the Default MethodInfo object.
        MI = &DefaultMethodInfo::DM;
      } else {
        MI = I->second;
        // Add it to the map, so that we don't need to call dladdr again
        // on something that belongs to our code.
        Functions.insert(std::make_pair(ip, MI));
      }
    } else {
      // The method is jitted, and no-one has intercepted its compilation.
      // Just return the Default MethodInfo object.
      MI = &DefaultMethodInfo::DM;
    }
  } else {
    MI = I->second;
  }
  FunctionMapLock.release();
  return MI;
}

MethodInfo* VirtualMachine::IPToMethodInfo(void* ip) {
  MethodInfo* MI = RuntimeFunctions.IPToMethodInfo(ip);
  if (MI) return MI;
  
  MI = SharedRuntimeFunctions.IPToMethodInfo(ip);
  if (MI) return MI;

  MI = StaticFunctions.IPToMethodInfo(ip);
  if (MI != &DefaultMethodInfo::DM) return MI;

  MI = SharedStaticFunctions.IPToMethodInfo(ip);
  return MI;
}


void SharedStartFunctionMap::initialize() {
  CamlFrame* currentFrame =
    (CamlFrame*)dlsym(SELF_HANDLE, "camlVmkitoptimized__frametable");

  Dl_info info;
  void* previousPtr = 0;
  const char* previousName = 0;
  CamlFrame* previousFrame = currentFrame;
  StaticAllocator = new BumpPtrAllocator();

  if (currentFrame) {
    while (true) {
      if (!currentFrame->ReturnAddress) break;
      int res = dladdr(currentFrame->ReturnAddress, &info);
      if (res) {
        if (previousPtr && info.dli_saddr != previousPtr) {
          StaticCamlMethodInfo* MI =
            new(*StaticAllocator, "StaticCamlMethodInfo")
            StaticCamlMethodInfo(previousFrame, previousPtr, previousName);
          addMethodInfo(MI, previousPtr);
        }
        previousName = info.dli_sname;
        previousFrame = currentFrame;
        previousPtr = info.dli_saddr;
      }

      currentFrame = (CamlFrame*) ((char*)currentFrame + 
        (currentFrame->NumLiveOffsets % 2) * sizeof(uint16_t) +
        currentFrame->NumLiveOffsets * sizeof(uint16_t) +
        sizeof(void*) + sizeof(uint16_t) + sizeof(uint16_t));
    }   
  }
}

CamlMethodInfo::CamlMethodInfo(CamlFrame* C, void* ip) {
  if (!C) {
    MethodInfo* MI = VirtualMachine::SharedStaticFunctions.IPToMethodInfo(ip);
    if (MI != &DefaultMethodInfo::DM) {
      C = ((CamlMethodInfo*)MI)->CF;
    }
  }
  CF = C;
}

StartEndFunctionMap VirtualMachine::SharedRuntimeFunctions;
SharedStartFunctionMap VirtualMachine::SharedStaticFunctions;
