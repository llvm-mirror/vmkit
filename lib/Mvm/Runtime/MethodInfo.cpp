//===------- MethodInfo.cpp - Runtime information for methods -------------===//
//
//                        The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include "mvm/Allocator.h"
#include "mvm/MethodInfo.h"
#include "mvm/VirtualMachine.h"
#include "MvmGC.h"

#include <dlfcn.h>
#include <map>

using namespace mvm;

void CamlMethodInfo::scan(uintptr_t closure, void* ip, void* addr) {
  assert(CF != NULL);
  //uintptr_t spaddr = (uintptr_t)addr + CF->FrameSize + sizeof(void*);
  uintptr_t spaddr = ((uintptr_t*)addr)[0];
  for (uint16 i = 0; i < CF->NumLiveOffsets; ++i) {
    Collector::scanObject((void**)(spaddr + CF->LiveOffsets[i]), closure);
  }
}

void MethodInfo::print(void* ip, void* addr) {
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

struct CamlFrameDecoder {
  CamlFrames* frames ;
  uint32 currentDescriptor;
  CamlFrame* currentFrame;

  CamlFrameDecoder(CamlFrames* frames) {
    this->frames = frames;
    currentDescriptor = 0;
    currentFrame = frames->frames();
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

CamlFrame* CamlFrames::frames() const {
  intptr_t ptr = reinterpret_cast<intptr_t>(this) + sizeof(uint32_t);
  // If the frames structure was not 4-aligned, manually do it here.
  if (ptr & 2) {
    ptr -= sizeof(uint16_t);
  }
  return reinterpret_cast<CamlFrame*>(ptr);
}

static BumpPtrAllocator* StaticAllocator = NULL;

FunctionMap::FunctionMap(BumpPtrAllocator& allocator, CamlFrames** allFrames) {
  if (allFrames == NULL) return;
  Functions.resize(40000); // Make sure the cache is big enough.
  int i = 0;
  CamlFrames* frames = NULL;
  while ((frames = allFrames[i++]) != NULL) {
    CamlFrameDecoder decoder(frames);
    while (decoder.hasNext()) {
      CamlFrame* frame = decoder.next();
      CamlMethodInfo* MI =
          new(allocator, "CamlMethodInfo") CamlMethodInfo(frame);
      addMethodInfoNoLock(MI, frame->ReturnAddress);
    }
  }
}

MethodInfo* FunctionMap::IPToMethodInfo(void* ip) {
  FunctionMapLock.acquire();
  llvm::DenseMap<void*, MethodInfo*>::iterator I = Functions.find(ip);
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
  addMethodInfoNoLock(meth, ip);
  FunctionMapLock.release();
}


void FunctionMap::removeMethodInfos(void* owner) {
  FunctionMapLock.acquire();
  llvm::DenseSet<void*> toRemove;
  for (llvm::DenseMap<void*, MethodInfo*>::iterator i = Functions.begin(),
       e = Functions.end(); i != e;) {
    mvm::MethodInfo* MI = i->second;
    if (MI->Owner == owner) {
      toRemove.insert(i->first);
    }
  }

  for (llvm::DenseSet<void*>::iterator i = toRemove.begin(),
       e = toRemove.end(); i != e;) {
    Functions.erase(*i);
  }

  FunctionMapLock.release();
}
