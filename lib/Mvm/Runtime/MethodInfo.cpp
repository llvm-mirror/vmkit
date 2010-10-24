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
  assert(CF != NULL);
  //uintptr_t spaddr = (uintptr_t)addr + CF->FrameSize + sizeof(void*);
  uintptr_t spaddr = ((uintptr_t*)addr)[0];
  for (uint16 i = 0; i < CF->NumLiveOffsets; ++i) {
    Collector::scanObject((void**)(spaddr + CF->LiveOffsets[i]), closure);
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

struct CamlFrames {
  uint16_t NumDescriptors;
  CamlFrame frames[1];
};

struct CamlFrameDecoder {
  CamlFrames* frames ;
  uint32 currentDescriptor;
  CamlFrame* currentFrame;

  CamlFrameDecoder(CamlFrames* frames) {
    this->frames = frames;
    currentDescriptor = 0;
    currentFrame = &(frames->frames[0]);
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
  }

  CamlFrame* next() {
    assert(hasNext());
    CamlFrame* result = currentFrame;
    advance();
    return result;
  }
};

static BumpPtrAllocator* StaticAllocator = NULL;

FunctionMap::FunctionMap() {
  CamlFrames* frames =
    (CamlFrames*)dlsym(SELF_HANDLE, "camlVmkitoptimized__frametable");
  if (frames == NULL) return;
  
  StaticAllocator = new BumpPtrAllocator();
  CamlFrameDecoder decoder(frames);
  Dl_info info;
  while (decoder.hasNext()) {
    CamlFrame* frame = decoder.next();
    int res = dladdr(frame->ReturnAddress, &info);
    assert(res != 0 && "No frame");
    StaticCamlMethodInfo* MI = new(*StaticAllocator, "StaticCamlMethodInfo")
        StaticCamlMethodInfo(frame, info.dli_sname);
    addMethodInfo(MI, frame->ReturnAddress);
  }
}

MethodInfo* FunctionMap::IPToMethodInfo(void* ip) {
  FunctionMapLock.acquire();
  std::map<void*, MethodInfo*>::iterator I = Functions.find(ip);
  MethodInfo* res = NULL;
  if (I != Functions.end()) {
    res = I->second;
  } else {
    res = &DefaultMethodInfo::DM;
  }
  FunctionMapLock.release();
  return res;
}

void FunctionMap::addMethodInfo(MethodInfo* meth, void* ip) {
  FunctionMapLock.acquire();
  Functions.insert(std::make_pair(ip, meth));
  FunctionMapLock.release();
}


void FunctionMap::removeMethodInfos(void* owner) {
  FunctionMapLock.acquire();
  std::map<void*, mvm::MethodInfo*>::iterator temp;
  for (std::map<void*, mvm::MethodInfo*>::iterator i = Functions.begin(),
       e = Functions.end(); i != e;) {
    mvm::MethodInfo* MI = i->second;
    if (MI->Owner == owner) {
      temp = i;
      i++;
      Functions.erase(temp);
    }
  }
  FunctionMapLock.release();
}
