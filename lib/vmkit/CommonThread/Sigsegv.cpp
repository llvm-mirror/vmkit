//===----------- Sigsegv.cpp - Sigsegv default handling -------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "vmkit/MethodInfo.h"
#include "vmkit/System.h"
#include "vmkit/VirtualMachine.h"
#include "vmkit/Thread.h"

#include <csignal>
#include <cstdio>

using namespace vmkit;

namespace {
  class Handler {
    void* context;
  public:
    Handler(void* ucontext): context(ucontext) {}
    void UpdateRegistersForNPE();
    void UpdateRegistersForStackOverflow();
  };
}

#if defined(ARCH_X64) && defined(LINUX_OS)
#include "Sigsegv-linux-x64.inc"
#elif defined(ARCH_X86) && defined(LINUX_OS)
#include "Sigsegv-linux-x86.inc"
#elif defined(ARCH_X64) && defined(MACOS_OS)
#include "Sigsegv-macos-x64.inc"
#else
void Handler::UpdateRegistersForStackOverflow() {
  UNREACHABLE();
}

void Handler::UpdateRegistersForNPE() {
  UNREACHABLE();
}

bool System::SupportsHardwareNullCheck() {
  return false;
}

bool System::SupportsHardwareStackOverflow() {
  return false;
}
#endif

extern "C" void ThrowStackOverflowError(word_t ip) {
  vmkit::Thread* th = vmkit::Thread::get();
  vmkit::FrameInfo* FI = th->MyVM->IPToFrameInfo(ip);
  if (FI->Metadata == NULL) {
    fprintf(stderr, "Thread %p received a SIGSEGV: either the VM code or an external\n"
                    "native method is bogus. Aborting...\n", (void*)th);
    abort();
  } else {
    vmkit::Thread::get()->MyVM->stackOverflowError();
  }
  UNREACHABLE();
}

extern "C" void ThrowNullPointerException(word_t ip) {
  vmkit::Thread* th = vmkit::Thread::get();
  vmkit::FrameInfo* FI = th->MyVM->IPToFrameInfo(ip);
  if (FI->Metadata == NULL) {
    fprintf(stderr, "Thread %p received a SIGSEGV: either the VM code or an external\n"
                    "native method is bogus. Aborting...\n", (void*)th);
    abort();
  } else {
    vmkit::Thread::get()->MyVM->nullPointerException();
  }
  UNREACHABLE();
}

void sigsegvHandler(int n, siginfo_t *info, void *context) {
  Handler handler(context);
  vmkit::Thread* th = vmkit::Thread::get();
  word_t addr = (word_t)info->si_addr;
  if (th->IsStackOverflowAddr(addr)) {
    if (vmkit::System::SupportsHardwareStackOverflow()) {
      handler.UpdateRegistersForStackOverflow();
    } else {
      fprintf(stderr, "Stack overflow in VM code or in JNI code. If it is from\n"
                      "the VM, it is either from the JIT, the GC or the runtime."
                      "\nThis has to be fixed in the VM: VMKit makes sure that\n"
                      "the bottom of the stack is always available when entering"
                      "\nthe VM.\n");
      abort();
    }
  } else {
    if (vmkit::System::SupportsHardwareNullCheck()) {
      handler.UpdateRegistersForNPE();
    } else {
      fprintf(stderr, "Thread %p received a SIGSEGV: either the VM code or an external\n"
                      "native method is bogus. Aborting...\n", (void*)th);
      abort();
    }
  }
}
