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
}

void Handler::UpdateRegistersForNPE() {
  ((ucontext_t*)context)->uc_mcontext->__ss.__rdi = ((ucontext_t*)context)->uc_mcontext->__ss.__rip + 1;
  ((ucontext_t*)context)->uc_mcontext->__ss.__rip = (word_t)HandleNullPointer;
}

void Handler::UpdateRegistersForStackOverflow() {
  UNREACHABLE();
}

bool System::SupportsHardwareNullCheck() {
  return true;
}

bool System::SupportsHardwareStackOverflow() {
  return false;
}
