//---------- Sigsegv-macos-x64.inc - Sigsegv handling ---------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

extern "C" {
  void HandleNullPointer(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl HandleNullPointer\n"
  "_HandleNullPointer:\n"
    // Save the faulting address to fake a real method call.
    "pushq %rdi\n"
    "jmp   _ThrowNullPointerException\n"
    );

  void HandleStackOverflow(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl HandleStackOverflow\n"
  "_HandleStackOverflow:\n"
    "pushq %rbp\n"
    "movq %rsi, %rbp\n"
    "callq   _ThrowStackOverflowError\n"
    );
}

void Handler::UpdateRegistersForNPE() {
  ((ucontext_t*)context)->uc_mcontext->__ss.__rdi = ((ucontext_t*)context)->uc_mcontext->__ss.__rip + 1;
  ((ucontext_t*)context)->uc_mcontext->__ss.__rip = (word_t)HandleNullPointer;
}

void Handler::UpdateRegistersForStackOverflow() {
  word_t alt_stack = mvm::Thread::get()->GetAlternativeStackStart();
  ((ucontext_t*)context)->uc_mcontext->__ss.__rdi = System::GetIPFromCallerAddress(((ucontext_t*)context)->uc_mcontext->__ss.__rbp);
  ((ucontext_t*)context)->uc_mcontext->__ss.__rsi = ((ucontext_t*)context)->uc_mcontext->__ss.__rbp;
  ((ucontext_t*)context)->uc_mcontext->__ss.__rsp = alt_stack;
  ((ucontext_t*)context)->uc_mcontext->__ss.__rip = (word_t)HandleStackOverflow;
}

bool System::SupportsHardwareNullCheck() {
  return true;
}

bool System::SupportsHardwareStackOverflow() {
  return true;
}
