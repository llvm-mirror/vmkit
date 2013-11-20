//===---------- ctthread.cc - Thread implementation for VMKit -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"

#include "VmkitGC.h"
#include "vmkit/MethodInfo.h"
#include "vmkit/VirtualMachine.h"
#include "vmkit/Cond.h"
#include "vmkit/Locks.h"
#include "vmkit/Thread.h"

#include <cassert>
#include <cstdio>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

using namespace vmkit;

int Thread::kill(void* tid, int signo) {
  return pthread_kill((pthread_t)tid, signo);
}

int Thread::kill(int signo) {
  return pthread_kill((pthread_t)internalThreadID, signo);
}

void Thread::exit(int value) {
  pthread_exit((void*)(intptr_t)value);
}

void Thread::yield(void) {
  Thread* th = vmkit::Thread::get();
  if (th->isVmkitThread()) {
    if (th->doYield && !th->inRV) {
      th->MyVM->rendezvous.join();
    }
  }
  sched_yield();
}

void Thread::joinRVBeforeEnter() {
  MyVM->rendezvous.joinBeforeUncooperative(); 
}

void Thread::joinRVAfterLeave(word_t savedSP) {
  MyVM->rendezvous.joinAfterUncooperative(savedSP); 
}

void Thread::startKnownFrame(KnownFrame& F) {
  // Get the caller of this function
  word_t cur = System::GetCallFrameAddress();
  F.previousFrame = lastKnownFrame;
  F.currentFP = cur;
  // This is used as a marker.
  F.currentIP = 0;
  lastKnownFrame = &F;
}

void Thread::endKnownFrame() {
  assert(lastKnownFrame->currentIP == 0);
  lastKnownFrame = lastKnownFrame->previousFrame;
}

void Thread::startUnknownFrame(KnownFrame& F) {
  // Get the caller of this function
  word_t cur = System::GetCallFrameAddress();
  // Get the caller of the caller.
  cur = System::GetCallerCallFrame(cur);
  F.previousFrame = lastKnownFrame;
  F.currentFP = cur;
  F.currentIP = System::GetReturnAddressOfCallFrame(cur);
  lastKnownFrame = &F;
}

void Thread::endUnknownFrame() {
  assert(lastKnownFrame->currentIP != 0);
  lastKnownFrame = lastKnownFrame->previousFrame;
}

void Thread::internalThrowException() {
  LONGJMP(lastExceptionBuffer->buffer, 1);
}

void Thread::printBacktrace() {
  StackWalker Walker(this);

  while (FrameInfo* FI = Walker.get()) {
    MyVM->printMethod(FI, Walker.returnAddress, Walker.callFrameAddress);
    ++Walker;
  }
}

void Thread::getFrameContext(word_t* buffer) {
  vmkit::StackWalker Walker(this);
  uint32_t i = 0;

  while (word_t ip = *Walker) {
    buffer[i++] = ip;
    ++Walker;
  }
}

uint32_t Thread::getFrameContextLength() {
  vmkit::StackWalker Walker(this);
  uint32_t i = 0;

  while (*Walker) {
    ++i;
    ++Walker;
  }
  return i;
}

FrameInfo* StackWalker::get() {
  if (callFrameAddress == thread->baseSP) return 0;
  returnAddress = System::GetReturnAddressOfCallFrame(callFrameAddress);
  return thread->MyVM->IPToFrameInfo(returnAddress);
}

word_t StackWalker::operator*() {
  if (callFrameAddress == thread->baseSP) return 0;
  returnAddress = System::GetReturnAddressOfCallFrame(callFrameAddress);
  return returnAddress;
}

void StackWalker::operator++() {
  if (callFrameAddress != thread->baseSP) {
    assert((callFrameAddress < thread->baseSP) && "Corrupted stack");
    assert((callFrameAddress < System::GetCallerCallFrame(callFrameAddress)) && "Corrupted stack");
    if ((frame != NULL) && (callFrameAddress == frame->currentFP)) {
      assert(frame->currentIP == 0);
      frame = frame->previousFrame;
      assert(frame != NULL);
      assert(frame->currentIP != 0);
      callFrameAddress = frame->currentFP;
      frame = frame->previousFrame;
    } else {
      callFrameAddress = System::GetCallerCallFrame(callFrameAddress);
    }
  }
}

StackWalker::StackWalker(vmkit::Thread* th) :
	returnAddress(0)
{
  thread = th;
  frame = th->lastKnownFrame;
  if (vmkit::Thread::get() == th) {
    callFrameAddress = System::GetCallFrameAddress();
    callFrameAddress = System::GetCallerCallFrame(callFrameAddress);
  } else {
    callFrameAddress = th->waitOnSP();
    if (frame) {
//    	if (frame->currentFP < addr) {
//    		fprintf(stderr, "Error in thread with pointer %p because %x < %x\n", th, frame->currentFP, addr);
//    		addr = frame->currentFP;
//    	}


    	assert(frame->currentFP >= callFrameAddress);
    }
    if (frame && (callFrameAddress == frame->currentFP)) {
      frame = frame->previousFrame;
      // Let this be called from JNI, as in
      // OpenJDK's JVM_FillInStackTrace:
      if (frame && frame->currentIP != 0)
        frame = frame->previousFrame;
      assert((frame == NULL) || (frame->currentIP == 0));
    }
  }
  assert(callFrameAddress && "No address to start with");
}

StackWalker::StackWalker() :
	returnAddress(0)
{
  thread = vmkit::Thread::get();
  frame = thread->lastKnownFrame;
  callFrameAddress = System::GetCallFrameAddress();
  callFrameAddress = System::GetCallerCallFrame(callFrameAddress);
  assert(callFrameAddress && "No address to start with");
}

void Thread::scanStack(word_t closure) {
  StackWalker Walker(this);
  while (FrameInfo* MI = Walker.get()) {
    MethodInfoHelper::scan(closure, MI, Walker.returnAddress, Walker.callFrameAddress);
    ++Walker;
  }
}

void Thread::enterUncooperativeCode(uint16_t level) {
  if (isVmkitThread()) {
  	if (!inRV) {
  		assert(!lastSP && "SP already set when entering uncooperative code");
      // Get the caller.
      word_t temp = System::GetCallFrameAddress();
      // Make sure to at least get the caller of the caller.
      ++level;
      while (level--)
    	  temp = System::GetCallerCallFrame(temp);
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, 0, temp);
      if (doYield) joinRVBeforeEnter();
      assert(lastSP && "No last SP when entering uncooperative code");
    }
  }
}

void Thread::enterUncooperativeCode(word_t SP) {
  if (isVmkitThread()) {
  	if (!inRV) {
      assert(!lastSP && "SP already set when entering uncooperative code");
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, 0, SP);
      if (doYield) joinRVBeforeEnter();
      assert(lastSP && "No last SP when entering uncooperative code");
    }
  }
}

void Thread::leaveUncooperativeCode() {
  if (isVmkitThread()) {
  	if (!inRV) {
      assert(lastSP && "No last SP when leaving uncooperative code");
      word_t savedSP = lastSP;
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, lastSP, 0);
      // A rendezvous has just been initiated, join it.
      if (doYield) joinRVAfterLeave(savedSP);
      assert(!lastSP && "SP has a value after leaving uncooperative code");
    }
  }
}

word_t Thread::waitOnSP() {
  // First see if we can get lastSP directly.
  word_t sp = lastSP;
  if (sp) return sp;
  
  // Then loop a fixed number of iterations to get lastSP.
  for (uint32 count = 0; count < 1000; ++count) {
    sp = lastSP;
    if (sp) return sp;
  }
  
  // Finally, yield until lastSP is not set.
  while ((sp = lastSP) == 0) vmkit::Thread::yield();

  assert(sp != 0 && "Still no sp");
  return sp;
}


word_t Thread::baseAddr = 0;

// These could be set at runtime.
#define STACK_SIZE 0x100000
#define NR_THREADS 255

/// StackThreadManager - This class allocates all stacks for threads. Because
/// we want fast access to thread local data, and can not rely on platform
/// dependent thread local storage (eg pthread keys are inefficient, tls is
/// specific to Linux), we put thread local data at the bottom of the 
/// stack. A simple mask computes the thread local data , based on the current
/// stack pointer.
//
/// The stacks are allocated at boot time. They must all be in the memory range
/// 0x?0000000 and Ox(?+1)0000000, so that the thread local data can be computed
/// and threads have a unique ID.
///
class StackThreadManager {
public:
  word_t baseAddr;
  uint32 allocPtr;
  uint32 used[NR_THREADS];
  LockNormal stackLock;

  StackThreadManager() {
    baseAddr = 0;
    word_t ptr = kThreadStart;

    uint32 flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
    baseAddr = (word_t)mmap((void*)ptr, STACK_SIZE * NR_THREADS,
                               PROT_READ | PROT_WRITE, flags, -1, 0);

    if (baseAddr == (word_t) MAP_FAILED) {
      fprintf(stderr, "Can not allocate thread memory\n");
      abort();
    }
 
    // Protect the page after the alternative stack.
    uint32 pagesize = System::GetPageSize();
    for (uint32 i = 0; i < NR_THREADS; ++i) {
      word_t addr = baseAddr + (i * STACK_SIZE) + pagesize
        + vmkit::System::GetAlternativeStackSize();
      mprotect((void*)addr, pagesize, PROT_NONE);
    }

    memset((void*)used, 0, NR_THREADS * sizeof(uint32));
    allocPtr = 0;
    vmkit::Thread::baseAddr = baseAddr;
  }

  word_t allocate() {
    stackLock.lock();
    uint32 myIndex = 0;
    do {
      if (!used[myIndex]) {
        used[myIndex] = 1;
        break;
      }
      ++myIndex;
    } while (myIndex != NR_THREADS);
  
    stackLock.unlock();
    
    if (myIndex != NR_THREADS)
      return baseAddr + myIndex * STACK_SIZE;

    return 0;
  }

};


/// Static allocate a stack manager. In the future, this should be virtual
/// machine specific.
StackThreadManager TheStackManager;

extern void sigsegvHandler(int, siginfo_t*, void*);
extern void sigsTermHandler(int n, siginfo_t *info, void *context);

/// internalThreadStart - The initial function called by a thread. Sets some
/// thread specific data, registers the thread to the GC and calls the
/// given routine of th.
///
void Thread::internalThreadStart(vmkit::Thread* th) {
  th->baseSP  = System::GetCallFrameAddress();

#if JAVA_INTERFACE_CALL_STACK
    th->stackEmbeddedListHead[j3::StackEmbeddedListIntendedCaller] = NULL;
#endif

  // Set the alternate stack as the second page of the thread's
  // stack.
  stack_t st;
  st.ss_sp = (void*)th->GetAlternativeStackEnd();
  st.ss_flags = 0;
  st.ss_size = System::GetAlternativeStackSize();
  sigaltstack(&st, NULL);

  // Set the SIGSEGV handler to diagnose errors.
  struct sigaction sa;
  sigset_t mask;
  sigfillset(&mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sa.sa_mask = mask;
  sa.sa_sigaction = sigsegvHandler;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  // to handle termination
  st.ss_sp = (void*)th->GetAlternativeStackEnd();
  st.ss_flags = 0;
  st.ss_size = System::GetAlternativeStackSize();
  sigaltstack(&st, NULL);
  sigfillset(&mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sa.sa_mask = mask;
  sa.sa_sigaction = sigsTermHandler;
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  //sigaction(SIGTERM, &sa, NULL);

  assert(th->MyVM && "VM not set in a thread");
//  fprintf(stderr, "Thread %p has TID %ld\n", th,syscall(SYS_gettid) );
  th->MyVM->rendezvous.addThread(th);
  th->routine(th);
  th->MyVM->removeThread(th);
}



/// start - Called by the creator of the thread to run the new thread.
int Thread::start(void (*fct)(vmkit::Thread*)) {
  pthread_attr_t attributs;
  pthread_attr_init(&attributs);
  pthread_attr_setstack(&attributs, this, STACK_SIZE);
  routine = fct;
  // Make sure to add it in the list of threads before leaving this function:
  // the garbage collector wants to trace this thread.
  MyVM->addThread(this);
  int res = pthread_create((pthread_t*)(void*)(&internalThreadID), &attributs,
                           (void* (*)(void *))internalThreadStart, this);
  pthread_attr_destroy(&attributs);
  return res;
}


/// operator new - Get a stack from the stack manager. The Thread object
/// will be placed in the first page at the bottom of the stack. Hence
/// Thread objects can not exceed a page.
void* Thread::operator new(size_t sz) {
  assert(sz < (size_t)getpagesize() && "Thread local data too big");
  void* res = (void*)TheStackManager.allocate();
  // Make sure the thread information is cleared.
  if (res != NULL) memset(res, 0, sz);
  assert(res && "Thread cannot be allocated because there is no room available");
  return res;
}

/// releaseThread - Remove the stack of the thread from the list of stacks
/// in use.
void Thread::releaseThread(vmkit::Thread* th) {
  // It seems like the pthread implementation in Linux is clearing with NULL
  // the stack of the thread. So we have to get the thread id before
  // calling pthread_join.
  void* thread_id = th->internalThreadID;
  if (thread_id != NULL) {
    // Wait for the thread to die.
    pthread_join((pthread_t)thread_id, NULL);
  }
  word_t index = ((word_t)th & System::GetThreadIDMask());
  index = (index & ~TheStackManager.baseAddr) >> 20;
  TheStackManager.used[index] = 0;
}

void Thread::throwNullPointerException(word_t methodIP)
{
	vmkit::FrameInfo* FI = MyVM->IPToFrameInfo(methodIP);
	if (FI->Metadata == NULL) {
		fprintf(stderr, "Thread %p received a SIGSEGV: either the VM code or an external\n"
					"native method is bogus. Aborting...\n", (void*)this);
		abort();
	}

	MyVM->nullPointerException();
	UNREACHABLE();
}
