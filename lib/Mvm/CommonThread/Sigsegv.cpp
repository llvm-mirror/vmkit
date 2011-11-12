//===----------- Sigsegv.cpp - Sigsegv default handling -------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "mvm/MethodInfo.h"
#include "mvm/System.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

#include <csignal>
#include <cstdio>

using namespace mvm;

#if defined(ARCH_X64) && defined(LINUX_OS)

extern "C" {
  void NativeHandleSignal(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl NativeHandleSignal\n"
  "NativeHandleSignal:\n"
    // Save The faulting address to fake a reall method call
    "pushq %rdi\n"
    "jmp   ThrowNullPointerException\n"
    );
}

#define UPDATE_REGS() \
    ((ucontext_t*)context)->uc_mcontext.gregs[REG_RDI] = ((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP] + 1; \
    ((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP] = (word_t)NativeHandleSignal;

#elif defined(ARCH_X64) && defined(MACOS_OS)
extern "C" {
  void NativeHandleSignal(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl NativeHandleSignal\n"
  "_NativeHandleSignal:\n"
    // Save The faulting address to fake a reall method call
    "pushq %rdi\n"
    "jmp   _ThrowNullPointerException\n"
    );
}

#define UPDATE_REGS() \
    ((ucontext_t*)context)->uc_mcontext->__ss.__rdi = ((ucontext_t*)context)->uc_mcontext->__ss.__rip + 1; \
    ((ucontext_t*)context)->uc_mcontext->__ss.__rip = (word_t)NativeHandleSignal;
#else
#define UPDATE_REGS() UNIMPLEMENTED();
#endif


extern "C" void ThrowNullPointerException(word_t ip) {
  mvm::Thread* th = mvm::Thread::get();
  mvm::FrameInfo* FI = th->MyVM->IPToFrameInfo(ip);
  if (FI->Metadata == NULL) {
    fprintf(stderr, "Thread %p received a SIGSEGV: either the VM code or an external\n"
                    "native method is bogus. Aborting...\n", (void*)th);
    abort();
  }
  mvm::Thread::get()->MyVM->nullPointerException();
  UNREACHABLE();
}

void sigsegvHandler(int n, siginfo_t *_info, void *context) {
  mvm::Thread* th = mvm::Thread::get();
  word_t addr = (word_t)_info->si_addr;
  if (addr > (word_t)th->getThreadID() && addr < (word_t)th->baseSP) {
    fprintf(stderr, "Stack overflow in VM code or in JNI code. If it is from\n"
                    "the VM, it is either from the JIT, the GC or the runtime."
                    "\nThis has to be fixed in the VM: VMKit makes sure that\n"
                    "the bottom of the stack is always available when entering"
                    "\nthe VM.\n");
    abort();
  } else if (mvm::System::SupportsHardwareNullCheck()) {
    UPDATE_REGS();
  } else {
    abort();
  }
}
