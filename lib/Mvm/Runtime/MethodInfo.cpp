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
#include "mvm/VirtualMachine.h"
#include "MvmGC.h"

#include <dlfcn.h>
#include <map>

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif

using namespace mvm;

void CamlMethodInfo::scan(uintptr_t closure, void* ip, void* addr) {
  if (!CF && InstructionPointer) {
    MethodInfo* MI = VirtualMachine::SharedStaticFunctions.IPToMethodInfo(ip);
    if (MI != &DefaultMethodInfo::DM) {
      CF = ((CamlMethodInfo*)MI)->CF;
    }
  }

  if (CF) {
    //uintptr_t spaddr = (uintptr_t)addr + CF->FrameSize + sizeof(void*);
    uintptr_t spaddr = ((uintptr_t*)addr)[0];
    for (uint16 i = 0; i < CF->NumLiveOffsets; ++i) {
      Collector::scanObject((void**)(spaddr + CF->LiveOffsets[i]), closure);
    }
  }
}

void StaticCamlMethodInfo::print(void* ip, void* addr) {
  fprintf(stderr, "; %p (%p) in %s static method\n", ip, addr, name);
}

void DefaultMethodInfo::print(void* ip, void* addr) {
  Dl_info info;
  int res = dladdr(ip, &info);
  if (res != 0 && info.dli_sname != NULL) {
    fprintf(stderr, "; %p (%p) in %s\n",  ip, addr, info.dli_sname);
  } else {
    fprintf(stderr, "; %p in Unknown method\n",  ip);
  }
}

DefaultMethodInfo DefaultMethodInfo::DM;

void DefaultMethodInfo::scan(uintptr_t closure, void* ip, void* addr) {
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
      }
    } else {
      // The method is jitted, and no-one has intercepted its compilation.
      // Just return the Default MethodInfo object.
      MI = &DefaultMethodInfo::DM;
    }
    // Add it to the map, so that we don't need to call dladdr again.
    Functions.insert(std::make_pair(ip, MI));
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

struct CamlFrames {
  uint16_t NumDescriptors;
  CamlFrame frames[1];
};

struct CamlFrameDecoder {
  CamlFrames* frames ;
  uint32 currentDescriptor;
  CamlFrame* currentFrame;
  Dl_info info;

  CamlFrameDecoder(CamlFrames* frames) {
    this->frames = frames;
    currentDescriptor = 0;
    currentFrame = &(frames->frames[0]);
    int res = dladdr(currentFrame->ReturnAddress, &info);
    assert(res != 0 && "No frame");
  }

  bool hasNext() {
    return currentDescriptor < frames->NumDescriptors;
  }

  void advance() {
    ++currentDescriptor;
    if (!hasNext()) return;
    currentFrame = (CamlFrame*) ((char*)currentFrame + 
      (currentFrame->NumLiveOffsets % 2) * sizeof(uint16_t) +
      currentFrame->NumLiveOffsets * sizeof(uint16_t) +
      sizeof(void*) + sizeof(uint16_t) + sizeof(uint16_t));
    int res = dladdr(currentFrame->ReturnAddress, &info);
    assert(res != 0 && "No frame");
  }

  CamlFrame* next(void** funcAddress, const char** funcName) {
    assert(hasNext());
    CamlFrame* result = currentFrame;
    *funcAddress = info.dli_saddr;
    *funcName = info.dli_sname;

    // Skip the remaining ones.
    do {
      advance();
    } while (hasNext() && (info.dli_saddr == *funcAddress));

    // Skip the entries that start at another method.
    while (hasNext() && (info.dli_saddr == currentFrame->ReturnAddress)) {
      advance();
    }

    while (hasNext() && (info.dli_saddr == NULL)) {
      advance();
    }
    return result;
  }
};

void SharedStartFunctionMap::initialize() {
  CamlFrames* frames =
    (CamlFrames*)dlsym(SELF_HANDLE, "camlVmkitoptimized__frametable");
  StaticAllocator = new BumpPtrAllocator();
  const char* name = NULL;
  void* address = NULL;

  if (frames != NULL) {
    CamlFrameDecoder decoder(frames);
    while (decoder.hasNext()) {
      CamlFrame* frame = decoder.next(&address, &name);
      StaticCamlMethodInfo* MI = new(*StaticAllocator, "StaticCamlMethodInfo")
          StaticCamlMethodInfo(frame, address, name);
      addMethodInfo(MI, address);
    }
  }
}

CamlMethodInfo::CamlMethodInfo(CamlFrame* C, void* ip) {
  InstructionPointer = ip;
  CF = C;
}

StartEndFunctionMap VirtualMachine::SharedRuntimeFunctions;
SharedStartFunctionMap VirtualMachine::SharedStaticFunctions;
