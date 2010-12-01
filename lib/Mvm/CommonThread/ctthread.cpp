//===---------- ctthread.cc - Thread implementation for VMKit -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "debug.h"

#include "mvm/GC.h"
#include "mvm/MethodInfo.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"
#include "mvm/VMKit.h"

#include <cassert>
#include <csetjmp>
#include <cstdio>
#include <ctime>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

using namespace mvm;

void Thread::tracer(uintptr_t closure) {
	mvm::Collector::markAndTraceRoot(&pendingException, closure);
	vmData->tracer(closure);
}

int Thread::kill(void* tid, int signo) {
  return pthread_kill((pthread_t)tid, signo);
}

int Thread::kill(int signo) {
  return pthread_kill((pthread_t)internalThreadID, signo);
}

void Thread::exit(int value) {
  pthread_exit((void*)value);
}

void Thread::yield(void) {
  Thread* th = mvm::Thread::get();
  if (th->isMvmThread()) {
    if (th->doYield && !th->inRV) {
      th->vmkit->rendezvous.join();
    }
  }
  sched_yield();
}

void Thread::joinRVBeforeEnter() {
  vmkit->rendezvous.joinBeforeUncooperative(); 
}

void Thread::joinRVAfterLeave(void* savedSP) {
  vmkit->rendezvous.joinAfterUncooperative(savedSP); 
}

void Thread::startKnownFrame(KnownFrame& F) {
  // Get the caller of this function
  void** cur = (void**)FRAME_PTR();
  // Get the caller of the caller.
  cur = (void**)cur[0];
  F.previousFrame = lastKnownFrame;
  F.currentFP = cur;
  // This is used as a marker.
  F.currentIP = NULL;
  lastKnownFrame = &F;
}

void Thread::endKnownFrame() {
  assert(lastKnownFrame->currentIP == NULL);
  lastKnownFrame = lastKnownFrame->previousFrame;
}

void Thread::startUnknownFrame(KnownFrame& F) {
  // Get the caller of this function
  void** cur = (void**)FRAME_PTR();
  // Get the caller of the caller.
  cur = (void**)cur[0];
  F.previousFrame = lastKnownFrame;
  F.currentFP = cur;
  F.currentIP = FRAME_IP(cur);
  lastKnownFrame = &F;
}

void Thread::endUnknownFrame() {
  assert(lastKnownFrame->currentIP != NULL);
  lastKnownFrame = lastKnownFrame->previousFrame;
}

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif

Thread* Thread::setPendingException(gc *obj) {
	llvm_gcroot(obj, 0);
  assert(pendingException == 0 && "pending exception already there?");
	pendingException = obj;
	return this;
}

void Thread::throwIt() {
  assert(pendingException);

#ifdef RUNTIME_DWARF_EXCEPTIONS
  // Use dlsym instead of getting the functions statically with extern "C"
  // because gcc compiles exceptions differently.
  typedef void* (*cxa_allocate_exception_type)(unsigned);
  typedef void  (*cxa_throw_type)(void*, void*, void*);
  
  static cxa_allocate_exception_type cxa_allocate_exception =
    (cxa_allocate_exception_type)(uintptr_t)
    dlsym(SELF_HANDLE, "__cxa_allocate_exception");
  
  static cxa_throw_type cxa_throw =
    (cxa_throw_type)(uintptr_t)
    dlsym(SELF_HANDLE, "__cxa_throw");
  
  void* exc = cxa_allocate_exception(0);
  // 32 = sizeof(_Unwind_Exception) in libgcc...  
  internalPendingException = (void*)((uintptr_t)exc - 32);
  cxa_throw(exc, 0, 0);
#else
#if defined(__MACH__)
  _longjmp(lastExceptionBuffer->buffer, 1);
#else
  longjmp(lastExceptionBuffer->buffer, 1);
#endif
#endif
}

void Thread::printBacktrace() {
  StackWalker Walker(this);

  while (MethodInfo* MI = Walker.get()) {
    MI->print(Walker.ip, Walker.addr);
    ++Walker;
  }
}

void Thread::getFrameContext(void** buffer) {
  mvm::StackWalker Walker(this);
  uint32_t i = 0;

  while (void* ip = *Walker) {
    buffer[i++] = ip;
    ++Walker;
  }
}

uint32_t Thread::getFrameContextLength() {
  mvm::StackWalker Walker(this);
  uint32_t i = 0;

  while (*Walker) {
    ++i;
    ++Walker;
  }
  return i;
}

MethodInfo* StackWalker::get() {
  if (addr == thread->baseSP) return 0;
  ip = FRAME_IP(addr);
  bool isStub = ((unsigned char*)ip)[0] == 0xCE;
  if (isStub) ip = addr[2];
  return thread->MyVM->IPToMethodInfo(ip);
}

void* StackWalker::operator*() {
  if (addr == thread->baseSP) return 0;
  ip = FRAME_IP(addr);
  bool isStub = ((unsigned char*)ip)[0] == 0xCE;
  if (isStub) ip = addr[2];
  return ip;
}

void StackWalker::operator++() {
  if (addr != thread->baseSP) {
    assert((addr < thread->baseSP) && "Corrupted stack");
    assert((addr < addr[0]) && "Corrupted stack");
    if ((frame != NULL) && (addr == frame->currentFP)) {
      assert(frame->currentIP == NULL);
      frame = frame->previousFrame;
      assert(frame != NULL);
      assert(frame->currentIP != NULL);
      addr = (void**)frame->currentFP;
      frame = frame->previousFrame;
    } else {
      addr = (void**)addr[0];
    }
  }
}

StackWalker::StackWalker(mvm::Thread* th) {
  thread = th;
  frame = th->lastKnownFrame;
  if (mvm::Thread::get() == th) {
    addr = (void**)FRAME_PTR();
    addr = (void**)addr[0];
  } else {
    addr = (void**)th->waitOnSP();
    if (frame) {
      assert(frame->currentFP >= addr);
    }
    if (frame && (addr == frame->currentFP)) {
      frame = frame->previousFrame;
      assert((frame == NULL) || (frame->currentIP == NULL));
    }
  }
  assert(addr && "No address to start with");
}


#ifdef WITH_LLVM_GCC
void Thread::scanStack(uintptr_t closure) {
  StackWalker Walker(this);
  while (MethodInfo* MI = Walker.get()) {
    MI->scan(closure, Walker.ip, Walker.addr);
    ++Walker;
  }
}

#else

void Thread::scanStack(uintptr_t closure) {
  register unsigned int  **max = (unsigned int**)(void*)this->baseSP;
  if (mvm::Thread::get() != this) {
    register unsigned int  **cur = (unsigned int**)this->waitOnSP();
    for(; cur<max; cur++) Collector::scanObject((void**)cur, closure);
  } else {
    jmp_buf buf;
    setjmp(buf);
    register unsigned int  **cur = (unsigned int**)&buf;
    for(; cur<max; cur++) Collector::scanObject((void**)cur, closure);
  }
}
#endif

void Thread::enterUncooperativeCode(unsigned level) {
  if (isMvmThread()) {
    if (!inRV) {
      assert(!lastSP && "SP already set when entering uncooperative code");
      // Get the caller.
      void* temp = FRAME_PTR();
      // Make sure to at least get the caller of the caller.
      ++level;
      while (level--) temp = ((void**)temp)[0];
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, 0, temp);
      if (doYield) joinRVBeforeEnter();
      assert(lastSP && "No last SP when entering uncooperative code");
    }
  }
}

void Thread::enterUncooperativeCode(void* SP) {
  if (isMvmThread()) {
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
  if (isMvmThread()) {
    if (!inRV) {
      assert(lastSP && "No last SP when leaving uncooperative code");
      void* savedSP = lastSP;
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, lastSP, 0);
      // A rendezvous has just been initiated, join it.
      if (doYield) joinRVAfterLeave(savedSP);
      assert(!lastSP && "SP has a value after leaving uncooperative code");
    }
  }
}

void* Thread::waitOnSP() {
  // First see if we can get lastSP directly.
  void* sp = lastSP;
  if (sp) return sp;
  
  // Then loop a fixed number of iterations to get lastSP.
  for (uint32 count = 0; count < 1000; ++count) {
    sp = lastSP;
    if (sp) return sp;
  }
  
  // Finally, yield until lastSP is not set.
  while ((sp = lastSP) == NULL) mvm::Thread::yield();

  assert(sp != NULL && "Still no sp");
  return sp;
}


uintptr_t Thread::baseAddr = 0;

// These could be set at runtime.
#define STACK_SIZE 0x100000
#define NR_THREADS 255

#if (__WORDSIZE == 64)
#define START_ADDR 0x110000000
#define END_ADDR 0x170000000
#else
#define START_ADDR 0x10000000
#define END_ADDR 0x70000000
#endif

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
  uintptr_t baseAddr;
  uint32 allocPtr;
  uint32 used[NR_THREADS];
  LockNormal stackLock;

  StackThreadManager() {
    baseAddr = 0;
    uintptr_t ptr = START_ADDR;

    // Do an mmap at a fixed address. If the mmap fails for a given address
    // use the next one.
    while (!baseAddr && ptr != END_ADDR) {
      ptr = ptr + 0x10000000;
#if defined (__MACH__)
      uint32 flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
#else
      uint32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
      baseAddr = (uintptr_t)mmap((void*)ptr, STACK_SIZE * NR_THREADS, 
                                 PROT_READ | PROT_WRITE, flags, -1, 0);
      if (baseAddr == (uintptr_t)MAP_FAILED) baseAddr = 0;
    }
    if (!baseAddr) {
      fprintf(stderr, "Can not allocate thread memory\n");
      abort();
    }
 
    // Protect the page after the first page. The first page contains thread
    // specific data. The second page has no access rights to catch stack
    // overflows.
    uint32 pagesize = getpagesize();
    for (uint32 i = 0; i < NR_THREADS; ++i) {
      uintptr_t addr = baseAddr + (i * STACK_SIZE) + pagesize;
      mprotect((void*)addr, pagesize, PROT_NONE);
    }

    memset((void*)used, 0, NR_THREADS * sizeof(uint32));
    allocPtr = 0;
    mvm::Thread::baseAddr = baseAddr;
  }

  uintptr_t allocate() {
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

/// internalThreadStart - The initial function called by a thread. Sets some
/// thread specific data, registers the thread to the GC and calls the
/// given routine of th.
///
void Thread::internalThreadStart(mvm::Thread* th) {
  th->baseSP  = FRAME_PTR();

  // Set the SIGSEGV handler to diagnose errors.
  struct sigaction sa;
  sigset_t mask;
  sigfillset(&mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_mask = mask;
  sa.sa_sigaction = sigsegvHandler;
  sigaction(SIGSEGV, &sa, NULL);


  assert(th->MyVM && "VM not set in a thread");
#ifdef ISOLATE
  th->IsolateID = th->MyVM->IsolateID;
#endif
  th->vmkit->rendezvous.prepareForJoin();
  th->routine(th);
  th->vmkit->rendezvous.removeThread(th);
}



/// start - Called by the creator of the thread to run the new thread.
/// The thread is in a detached state, because each virtual machine has
/// its own way of waiting for created threads.
int Thread::start(void (*fct)(mvm::Thread*)) {
  pthread_attr_t attributs;
  pthread_attr_init(&attributs);
  pthread_attr_setstack(&attributs, this, STACK_SIZE);
  routine = fct;
  // Make sure to add it in the list of threads before leaving this function:
  // the garbage collector wants to trace this thread.
  vmkit->rendezvous.addThread(this);
  int res = pthread_create((pthread_t*)(void*)(&internalThreadID), &attributs,
                           (void* (*)(void *))internalThreadStart, this);
  pthread_detach((pthread_t)internalThreadID);
  pthread_attr_destroy(&attributs);
  return res;
}


/// operator new - Get a stack from the stack manager. The Thread object
/// will be placed in the first page at the bottom of the stack. Hence
/// Thread objects can not exceed a page.
void* Thread::operator new(size_t sz) {
  assert(sz < (size_t)getpagesize() && "Thread local data too big");
  void* res = (void*)TheStackManager.allocate();
  // Give it a second chance.
  if (res == NULL) {
    Collector::collect();
    // Wait for the finalizer to have cleaned up the threads.
    while (res == NULL) {
      mvm::Thread::yield();
      res = (void*)TheStackManager.allocate();
    }
  }
  // Make sure the thread information is cleared.
  memset(res, 0, sz);
  return res;
}

/// releaseThread - Remove the stack of the thread from the list of stacks
/// in use.
void Thread::releaseThread(mvm::Thread* th) {
  // It seems like the pthread implementation in Linux is clearing with NULL
  // the stack of the thread. So we have to get the thread id before
  // calling pthread_join.
  void* thread_id = th->internalThreadID;
  if (thread_id != NULL) {
    // Wait for the thread to die.
    pthread_join((pthread_t)thread_id, NULL);
  }
  uintptr_t index = ((uintptr_t)th & Thread::IDMask);
  index = (index & ~TheStackManager.baseAddr) >> 20;
  TheStackManager.used[index] = 0;
}
