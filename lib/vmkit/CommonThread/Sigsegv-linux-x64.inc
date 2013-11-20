//===-------- Sigsegv-linux-x64.inc - Sigsegv handling --------------------===//
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
  "HandleNullPointer:\n"
    // Save the faulting address to fake a real method call.
    "pushq %rdi\n"
    "jmp   ThrowNullPointerException\n"
    );
  
  void HandleStackOverflow(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl HandleStackOverflow\n"
  "HandleStackOverflow:\n"
    "pushq %rbp\n"
    "movq %rsi, %rbp\n"
    "callq   ThrowStackOverflowError\n"
    );
}

void Handler::UpdateRegistersForNPE() {
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RDI] = ((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP] + 1;
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP] = (word_t)HandleNullPointer;
}

void Handler::UpdateRegistersForStackOverflow() {
  word_t alt_stack = vmkit::Thread::get()->GetAlternativeStackStart();
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RDI] = System::GetReturnAddressOfCallFrame(((ucontext_t*)context)->uc_mcontext.gregs[REG_RBP]);
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RSI] = ((ucontext_t*)context)->uc_mcontext.gregs[REG_RBP];
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RSP] = alt_stack;
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP] = (word_t)HandleStackOverflow;
}

bool System::SupportsHardwareNullCheck() {
  return true;
}

bool System::SupportsHardwareStackOverflow() {
  return true;
}
