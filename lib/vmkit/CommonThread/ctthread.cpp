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

#include <iostream>
#include <cassert>
#include <cstdio>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

using namespace vmkit;

ExceptionBuffer::ExceptionBuffer()
{
	addToThreadExceptionList(0);
	handlerIsolateID = Thread::get()->getIsolateID();
}

void ExceptionBuffer::addToThreadExceptionList(void* returnAddr)
{
  Thread* th = Thread::get();
  handlerMethod = returnAddr;
  previousBuffer = th->lastExceptionBuffer;
  th->lastExceptionBuffer = this;
}

void ExceptionBuffer::removeFromThreadExceptionList()
{
  Thread* th = Thread::get();
  assert(th->lastExceptionBuffer == this && "Wrong exception buffer");
  th->lastExceptionBuffer = previousBuffer;
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

void Thread::joinRVAfterLeave(void* savedSP) {
  MyVM->rendezvous.joinAfterUncooperative(savedSP); 
}

void Thread::startKnownFrame(KnownFrame& F) {
  // Get the caller of this function
  void* cur = StackWalker_getCallFrameAddress();
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
  void* cur = StackWalker_getCallFrameAddress();
  // Get the caller of the caller.
  cur = StackWalker::getCallerCallFrameAddress(cur);
  F.previousFrame = lastKnownFrame;
  F.currentFP = cur;
  F.currentIP = StackWalker::getReturnAddressFromCallFrame(cur);
  lastKnownFrame = &F;
}

void Thread::endUnknownFrame() {
  assert(lastKnownFrame->currentIP != 0);
  lastKnownFrame = lastKnownFrame->previousFrame;
}

void Thread::internalThrowException() {
  LONGJMP(lastExceptionBuffer->getSetJmpBuffer(), 1);
}

void Thread::printBacktrace() {
  StackWalker Walker(this);

  while (FrameInfo* FI = Walker.get()) {
    MyVM->printMethod(FI, Walker.getReturnAddress(), Walker.getCallFrame());
    ++Walker;
  }
}

void Thread::getFrameContext(void** buffer) {
  vmkit::StackWalker Walker(this);
  uint32_t i = 0;

  while (void* ip = *Walker) {
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

void* StackWalker::getCallerCallFrameAddress(void* callFrame)
{
	void **oldBasePtr = (void**)callFrame;
	return *oldBasePtr;
}

void** StackWalker::getReturnAddressPtrFromCallFrame(void* callFrame)
{
	void **oldBasePtr = (void**)callFrame;

#if defined(MACOS_OS) && defined(ARCH_PPC)
    return oldBasePtr + 2;
#else
    return oldBasePtr + 1;
#endif
}

void* StackWalker::getReturnAddressFromCallFrame(void* callFrame)
{
	return *getReturnAddressPtrFromCallFrame(callFrame);
}

FrameInfo* StackWalker::get() {
  if (callFrame == thread->baseSP) return 0;
  return thread->MyVM->IPToFrameInfo(getReturnAddress());
}

StackWalkerState StackWalker::getState() const
{
    const FrameInfo *fi = this->get();
    if (!fi) return				StackWalkerInvalid;
    if (!fi->Metadata) return	StackWalkerValid;
    return						StackWalkerValidMetadata;
}

void* StackWalker::operator*() {
  if (callFrame == thread->baseSP) return 0;
  return getReturnAddress();
}

void* StackWalker::getCallerCallFrame() const
{
	StackWalker walker(*this);
	++walker;
	return walker.getCallFrame();
}

void StackWalker::operator++() {
  for (;;) {
    if (callFrame != thread->baseSP) {
      assert((callFrame < thread->baseSP) && "Corrupted stack");
      assert((callFrame < StackWalker::getCallerCallFrameAddress(callFrame)) && "Corrupted stack");
      if ((frame != NULL) && (callFrame == frame->currentFP)) {
        assert(frame->currentIP == 0);
        frame = frame->previousFrame;
        assert(frame != NULL);
        assert(frame->currentIP != 0);
        callFrame = frame->currentFP;
        frame = frame->previousFrame;
      } else {
        callFrame = StackWalker::getCallerCallFrameAddress(callFrame);
      }
    }

    if (!onlyReportMetadataEnabledFrames) break;
    StackWalkerState state = getState();
    if (state == StackWalkerInvalid || state == StackWalkerValidMetadata) break;
  }
}

void StackWalker::operator--()
{
	// The call stack is a singly-linked list of call frames whose head is the last
	// called method frame. This means that implementing this feature (getting the
	// called frame of the current frame) requires rescanning the whole stack from the
	// beginning (the last called frame), which can be slow in some cases.

	StackWalker caller(*this, true);
	StackWalker called(caller);
	++caller;

	for (void* currentAddr = this->getCallFrame();
		(caller.get() != NULL) && (caller.getCallFrame() != currentAddr);
		called = caller, ++caller);

	assert((caller.get() != NULL) && "Caller of the current frame not found!");
	*this = called;
}

// This code must be a macro because it must be directly called
//from its caller, with not additional function frames in between.
#define StackWalker_reset()										\
{																\
	if (vmkit::Thread::get() == thread) {						\
		callFrame = StackWalker_getCallFrameAddress();			\
		callFrame = StackWalker::getCallerCallFrameAddress(callFrame);	\
	} else {													\
		callFrame = thread->waitOnSP();							\
		if (frame) assert(frame->currentFP >= callFrame);		\
		if (frame && (callFrame == frame->currentFP)) {			\
			frame = frame->previousFrame;						\
			if (frame && frame->currentIP != 0)					\
				frame = frame->previousFrame;					\
			assert((frame == NULL) || (frame->currentIP == 0));	\
		}														\
	}															\
	assert(callFrame && "No address to start with");			\
	if (onlyReportMetadataEnabledFrames) {						\
		FrameInfo *fi = this->get();							\
		if ((fi != NULL) && !fi->Metadata) ++(*(this));			\
	}															\
}

StackWalker::StackWalker(vmkit::Thread* th, bool only_report_metadata_enabled_frames) :
	callFrame(0), frame(th->lastKnownFrame), thread(th),
	onlyReportMetadataEnabledFrames(only_report_metadata_enabled_frames)
{
	StackWalker_reset();
}

StackWalker::StackWalker(const StackWalker& obj, bool reset) :
	callFrame(obj.callFrame), frame(obj.frame), thread(obj.thread),
	onlyReportMetadataEnabledFrames(obj.onlyReportMetadataEnabledFrames)
{
	if (!reset) return;
	StackWalker_reset();
}

void StackWalker::reset()
{
	StackWalker_reset();
}

StackWalker& StackWalker::operator = (const StackWalker& obj)
{
	callFrame = obj.callFrame;
	frame = obj.frame;
	thread = obj.thread;
	onlyReportMetadataEnabledFrames = obj.onlyReportMetadataEnabledFrames;
	return *this;
}

void* StackWalker::updateReturnAddress(void* newAddr)
{
	void** retAddrPtr = StackWalker::getReturnAddressPtrFromCallFrame(callFrame);
	void* oldRetAddr = *retAddrPtr;
	*retAddrPtr = newAddr;
	return oldRetAddr;
}

void* StackWalker::updateCallerFrameAddress(void* newAddr)
{
	void **oldBasePtr = (void**)callFrame;
	void* oldOldBasePtr = *oldBasePtr;

	for (void* framePtr = callFrame; framePtr != newAddr; framePtr = StackWalker::getCallerCallFrameAddress(framePtr)) {
		for (KnownFrame *kf = thread->lastKnownFrame, *pkf = NULL; kf != NULL; pkf = kf, kf = kf->previousFrame) {
			if (kf->currentFP != framePtr) continue;

			if (!pkf)
				thread->lastKnownFrame = kf->previousFrame;
			else
				pkf->previousFrame = kf->previousFrame;
		}
	}

	*oldBasePtr = newAddr;
	return oldOldBasePtr;
}

void StackWalker::dump() const
{
	thread->MyVM->printCallStack(*this);
}

void Thread::scanStack(word_t closure) {
  StackWalker Walker(this);
  while (FrameInfo* MI = Walker.get()) {
    MethodInfoHelper::scan(closure, MI, Walker.getReturnAddress(), Walker.getCallFrame());
    ++Walker;
  }
}

void Thread::enterUncooperativeCode(uint16_t level) {
  if (isVmkitThread()) {
    if (!inRV) {
      assert(!lastSP && "SP already set when entering uncooperative code");
      // Get the caller.
      void* temp = StackWalker_getCallFrameAddress();
      // Make sure to at least get the caller of the caller.
      ++level;
      while (level--) temp = StackWalker::getCallerCallFrameAddress(temp);
      // The cas is not necessary, but it does a memory barrier.
      __sync_bool_compare_and_swap(&lastSP, 0, temp);
      if (doYield) joinRVBeforeEnter();
      assert(lastSP && "No last SP when entering uncooperative code");
    }
  }
}

void Thread::enterUncooperativeCode(void* SP) {
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
//extern void interruptSignalHandler(int signal_number, siginfo_t *info, void *context);

/// internalThreadStart - The initial function called by a thread. Sets some
/// thread specific data, registers the thread to the GC and calls the
/// given routine of th.
///
void Thread::internalThreadStart(vmkit::Thread* th) {
  th->baseSP  = StackWalker_getCallFrameAddress();

  // Set the alternate stack as the second page of the thread's
  // stack.
  stack_t st;
  st.ss_sp = (void*)th->GetAlternativeStackEnd();
  st.ss_flags = 0;
  st.ss_size = System::GetAlternativeStackSize();
  sigaltstack(&st, NULL);

  // Set the SIGSEGV handler to diagnose errors.
  struct sigaction sa = {};
//  sigset_t mask;
//  sigfillset(&mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
//  sa.sa_mask = mask;
  sa.sa_sigaction = sigsegvHandler;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);

  assert(th->MyVM && "VM not set in a thread");
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

isolate_id_t Thread::getValidIsolateID(isolate_id_t isolateID)
{
	if (isolateID != CURRENT_ISOLATE) return isolateID;
	return Thread::get()->getIsolateID();
}

bool Thread::runsDeadIsolate() const
{
	return runningDeadIsolate;
}

void Thread::markRunningDeadIsolate()
{
	runningDeadIsolate = true;
}

void Thread::setIsolateID(isolate_id_t newIsolateID)
{
	isolateID = newIsolateID;
}

isolate_id_t Thread::getIsolateID() const
{
	return isolateID;
}

bool Thread::isCurrentThread()
{
	return (pthread_t)internalThreadID == pthread_self();
}

void Thread::throwNullPointerException(void* methodIP) const
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
