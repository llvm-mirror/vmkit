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
    "push %eax\n"
    // Save the faulting address to fake a real method call.
    "push %eax\n"
    "jmp   ThrowNullPointerException\n"
    );
  
  void HandleStackOverflow(void);
  asm(
    ".text\n"
    ".align 8\n"
    ".globl HandleStackOverflow\n"
  "HandleStackOverflow:\n"
    "push %ebp\n"
    "mov %eax, %ebp\n"
    "push %ebx\n"
    "call   ThrowStackOverflowError\n"
    );
}

void Handler::UpdateRegistersForNPE() {
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_EAX] = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EIP] + 1;
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_EIP] = (word_t)HandleNullPointer;
}

void Handler::UpdateRegistersForStackOverflow() {
  word_t alt_stack = vmkit::Thread::get()->GetAlternativeStackStart();
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_EBX] = System::GetReturnAddressOfCallFrame(((ucontext_t*)context)->uc_mcontext.gregs[REG_EBP]);
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_EAX] = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EBP];
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_ESP] = alt_stack;
  ((ucontext_t*)context)->uc_mcontext.gregs[REG_EIP] = (word_t)HandleStackOverflow;
}

bool System::SupportsHardwareNullCheck() {
  return true;
}

bool System::SupportsHardwareStackOverflow() {
  return true;
}
