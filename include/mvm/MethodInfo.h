//===---------- MethodInfo.h - Meta information for methods ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_METHODINFO_H
#define MVM_METHODINFO_H

#include "mvm/Allocator.h"
#include "mvm/GC/GC.h"

namespace mvm {

class FrameInfo {
public:
  void* Metadata;
  void* ReturnAddress;
  uint16_t SourceIndex;
  uint16_t FrameSize;
  uint16_t NumLiveOffsets;
  int16_t LiveOffsets[1];
};
 
class MethodInfoHelper {
public:
  static void print(void* ip, void* addr);

  static void scan(uintptr_t closure, FrameInfo* FI, void* ip, void* addr);
  
  static void* isStub(void* ip, void* addr) {
    bool isStub = ((unsigned char*)ip)[0] == 0xCE;
    if (isStub) ip = ((void**)addr)[2];
    return ip;
  }

  static uint32_t FrameInfoSize(uint32_t NumOffsets) {
    uint32_t FrameInfoSize = sizeof(FrameInfo) + (NumOffsets - 1) * sizeof(int16_t);
    if (FrameInfoSize & 2) {
      FrameInfoSize += sizeof(int16_t);
    }
    return FrameInfoSize;
  }
};


class Frames {
public:
  uint32_t NumDescriptors;
  FrameInfo* frames() const {
    return reinterpret_cast<FrameInfo*>(
        reinterpret_cast<uintptr_t>(this) + sizeof(Frames));
  }

  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator, uint32_t NumDescriptors, uint32_t NumOffsets) {
    Frames* res = reinterpret_cast<Frames*>(
        allocator.Allocate(sizeof(Frames) + NumDescriptors * MethodInfoHelper::FrameInfoSize(NumOffsets), "Frames"));
    assert((reinterpret_cast<uintptr_t>(res) & 2) == 0);
    return res;
  }
};

class CompiledFrames {
public:
  uint32_t NumCompiledFrames;
  Frames* frames() const {
    return reinterpret_cast<Frames*>(
        reinterpret_cast<uintptr_t>(this) + sizeof(CompiledFrames));
  }
};

class FrameIterator {
public:
  const Frames& frames;
  uint32 currentFrameNumber;
  FrameInfo* currentFrame;

  FrameIterator(const Frames& f)
      : frames(f), currentFrameNumber(0) , currentFrame(f.frames()) {
  }

  bool hasNext() {
    return currentFrameNumber < frames.NumDescriptors;
  }

  void advance(int NumLiveOffsets) {
    ++currentFrameNumber;
    if (!hasNext()) return;
    uintptr_t ptr =
      reinterpret_cast<uintptr_t>(currentFrame) + MethodInfoHelper::FrameInfoSize(NumLiveOffsets);
    currentFrame = reinterpret_cast<FrameInfo*>(ptr);
  }

  FrameInfo* next() {
    assert(hasNext());
    FrameInfo* result = currentFrame;
    advance(currentFrame->NumLiveOffsets);
    return result;
  }
};

} // end namespace mvm
#endif // MVM_METHODINFO_H
