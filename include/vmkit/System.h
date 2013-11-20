//===-------------------- System.h - System utils -------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef VMKIT_SYSTEM_H
#define VMKIT_SYSTEM_H

#include <csetjmp>
#include <cstring>
#include <dlfcn.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include "vmkit/config.h"

#if defined(__linux__) || defined(__FreeBSD__)
#define LINUX_OS 1
#elif defined(__APPLE__)
#define MACOS_OS 1
#else
#error OS detection failed.
#endif

#if (__WORDSIZE == 32)
#define ARCH_32 1
#elif (__WORDSIZE == 64)
#define ARCH_64 1
#elif defined(__LP64__)
#define ARCH_64 1
#else
#error Word size not supported.
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_X86 1
#elif defined (__PPC__) && ARCH_32
#define ARCH_PPC 1
#define ARCH_PPC_32 1
#elif defined (__PPC__) && ARCH_64
#define ARCH_PPC 1
#define ARCH_PPC_64 1
#else
#error Architecture detection failed.
#endif

#if ARCH_X64
typedef uint64_t word_t;
#else
typedef uint32_t word_t;
#endif

namespace vmkit {

const int kWordSize = sizeof(word_t);
const int kWordSizeLog2 = kWordSize == 4 ? 2 : 3;

#if ARCH_X64
const word_t kThreadStart   = 0x0000000110000000LL;
const word_t kThreadIDMask  = 0xFFFFFFFFFFF00000LL;
const word_t kVmkitThreadMask = 0xFFFFFFFFF0000000LL;
#else
const word_t kThreadStart   = 0x10000000;
const word_t kThreadIDMask  = 0x7FF00000;
const word_t kVmkitThreadMask = 0xF0000000;
#endif

#if MACOS_OS
  #define LONGJMP _longjmp
  #define SETJMP _setjmp
  #define DYLD_EXTENSION ".dylib"
  #define SELF_HANDLE RTLD_DEFAULT
#else
  #define LONGJMP longjmp
  #define SETJMP setjmp
  #define DYLD_EXTENSION ".so"
  #define SELF_HANDLE 0
#endif

#if MACOS_OS
  #if ARCH_X64
    const word_t kGCMemoryStart = 0x300000000LL;
  #else
    const word_t kGCMemoryStart = 0x30000000;
  #endif
#else
  const word_t kGCMemoryStart = 0x50000000;
#endif

const word_t kGCMemorySize = 0x30000000;

#define TRY { vmkit::ExceptionBuffer __buffer__; if (!SETJMP(__buffer__.buffer))
#define CATCH else
#define IGNORE else { vmkit::Thread::get()->clearException(); }}
#define END_CATCH }

class System {
public:
  static bool IsWordAligned(word_t ptr) {
    return (ptr & (kWordSize - 1)) == 0;
  }

  static word_t WordAlignUp(word_t ptr) {
    if (!IsWordAligned(ptr)) {
      return (ptr & ~(kWordSize - 1)) + kWordSize;
    }
    return ptr;
  }

  static bool IsPageAligned(word_t ptr) {
    return (ptr & (GetPageSize() - 1)) == 0;
  }

  static word_t PageAlignUp(word_t ptr) {
    if (!IsPageAligned(ptr)) {
      return (ptr & ~(GetPageSize() - 1)) + GetPageSize();
    }
    return ptr;
  }

  static word_t GetAlternativeStackSize() {
    static word_t size = PageAlignUp(SIGSTKSZ);
    return size;
  }

  // Apply this mask to the stack pointer to get the Thread object.
  static word_t GetThreadIDMask() {
    return kThreadIDMask;
  }

  // Apply this mask to verify that the current thread was created by vmkit.
  static word_t GetVmkitThreadMask() {
    return kVmkitThreadMask;
  }

  // Get the memory start of thread stack addresses.
  static word_t GetThreadStart() {
    return kThreadStart;
  }

  static uint32_t GetPageSize() {
    static uint32_t pagesize = getpagesize();
    return pagesize;
  }

  static word_t GetCallFrameAddress() __attribute((always_inline)) {
#if defined(ARCH_X86) || defined(ARCH_X64)
    return (word_t)__builtin_frame_address(0);
#else
    return ((word_t*)__builtin_frame_address(0))[0];
#endif
  }

  static word_t GetCallerCallFrame(word_t currentCallFrame) {
    return *(word_t*)currentCallFrame;
  }

  static word_t GetReturnAddressOfCallFrame(word_t currentCallFrame) {
#if defined(MACOS_OS) && defined(ARCH_PPC)
    return ((word_t*)currentCallFrame)[2];
#else
    return ((word_t*)currentCallFrame)[1];
#endif
  }

  static int SetJmp(jmp_buf buffer) {
#if defined(MACOS_OS)
    return _setjmp(buffer);
#else
    return setjmp(buffer);
#endif
  }

  static void LongJmp(jmp_buf buffer, int val) {
#if defined(MACOS_OS)
    _longjmp(buffer, val);
#else
    longjmp(buffer, val);
#endif
  }

  static void* GetSelfHandle() {
    return SELF_HANDLE;
  }

  static const char* GetDyLibExtension() {
    return DYLD_EXTENSION;
  }

  static double ReadDouble(int first, int second) {
    int values[2];
    double res[1];
#if ARCH_PPC
    values[0] = second;
    values[1] = first;
#else
    values[0] = first;
    values[1] = second;
#endif
    memcpy(res, values, 8); 
    return res[0];
  }

  static int64_t ReadLong(int first, int second) {
    int values[2];
    int64_t res[1];
#if ARCH_PPC
    values[0] = second;
    values[1] = first;
#else
    values[0] = first;
    values[1] = second;
#endif
    memcpy(res, values, 8); 
    return res[0];
  }

  static uint8_t* GetLastBytePtr(word_t ptr) {
#if ARCH_PPC
  return ((uint8_t*)ptr) + vmkit::kWordSize - 1;
#else
  return ((uint8_t*)ptr);
#endif
  }

  static int GetNumberOfProcessors() {
    return sysconf(_SC_NPROCESSORS_ONLN);
  }

  static void Exit(int value) {
    _exit(value);
  }

  static bool SupportsHardwareNullCheck();
  static bool SupportsHardwareStackOverflow();
};

}

#endif
