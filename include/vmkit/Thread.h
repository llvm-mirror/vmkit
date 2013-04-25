//===---------------- Threads.h - Micro-vm threads ------------------------===//
//
//                        The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef VMKIT_THREAD_H
#define VMKIT_THREAD_H

#include <cassert>
#include <cstdio>
#include <stdlib.h>

#include "debug.h"
#include "types.h"

#include "vmkit/System.h"

typedef uint32_t				isolate_id_t;
#define CURRENT_ISOLATE			((isolate_id_t)(-1))
#define IsValidIsolateID(iid)	((iid) != CURRENT_ISOLATE)

namespace vmkit {

class FrameInfo;
class VirtualMachine;
class Thread;


class KnownFrame {
public:
  void* currentFP;
  void* currentIP;
  KnownFrame* previousFrame;
};


enum StackWalkerState {
	StackWalkerInvalid = 0,
	StackWalkerValid,
	StackWalkerValidMetadata
};

/// StackWalker - This class walks the stack of threads, returning a FrameInfo
/// object at each iteration.
///
class StackWalker
{
protected:
  void* callFrame;
  KnownFrame* frame;
  vmkit::Thread* thread;
  bool onlyReportMetadataEnabledFrames;

public:
  StackWalker(vmkit::Thread* th, bool only_report_metadata_enabled_frames = false);
  StackWalker(const StackWalker& obj, bool reset = false);

  StackWalker& operator = (const StackWalker& obj);
  bool operator == (const StackWalker& obj) const {return (thread == obj.thread) && (callFrame == obj.callFrame);}
  bool operator != (const StackWalker& obj) const {return !((*this) == obj);}
  void operator++();
  void operator--();
  void* operator*();
  FrameInfo* get();
  const FrameInfo* get() const {return const_cast<StackWalker*>(this)->get();}
  void reset() __attribute__ ((noinline));

  void* getCallFrame() const				{return callFrame;}
  void* getCallerCallFrame() const;
  void* getReturnAddress() const			{return getReturnAddressFromCallFrame(callFrame);}
  KnownFrame* getKnownFrame()				{return frame;}
  vmkit::Thread* getScannedThread() const	{return thread;}
  StackWalkerState getState() const;
  bool isValid() const						{return getState() >= StackWalkerValid;}
  bool hasMetaData() const					{return getState() == StackWalkerValidMetadata;}

  void* updateReturnAddress(void* newAddr);
  void* updateCallerFrameAddress(void* newAddr);

  bool isReportingOnlyMetadataEnabledFrames() const {return onlyReportMetadataEnabledFrames;}
  void reportOnlyMetadataEnabledFrames(bool only_report_metadata_enabled_frames) {onlyReportMetadataEnabledFrames = only_report_metadata_enabled_frames;}

  void dump() const;

  static void* getCallerCallFrameAddress(void* callFrame);
  static void** getReturnAddressPtrFromCallFrame(void* callFrame);
  static void* getReturnAddressFromCallFrame(void* callFrame);

#if defined(ARCH_X86) || defined(ARCH_X64)
#define StackWalker_getCallFrameAddress()	((void*)(__builtin_frame_address(0)))
#else
#define StackWalker_getCallFrameAddress()	(*(void**)__builtin_frame_address(0))
#endif

};


class ExceptionBuffer {
protected:
	/*
		WARNING:
		- Do not change the fields order or type, unless you update LLVM data types.
		- Do not declare anything as virtual (to avoid generating a Virtual Table).
	*/
	void* handlerMethod;
	uint32_t handlerIsolateID;
	ExceptionBuffer* previousBuffer;
	jmp_buf buffer;

public:
  ExceptionBuffer();
  ~ExceptionBuffer() {removeFromThreadExceptionList();}

  void addToThreadExceptionList(void* returnAddr);
  void removeFromThreadExceptionList();

  void* getHandlerMethod() const {return handlerMethod;}
  isolate_id_t getHandlerIsolateID() const {return handlerIsolateID;}

  ExceptionBuffer* getPrevious() {return previousBuffer;}
  void setPrevious(ExceptionBuffer* newValue) {previousBuffer = newValue;}

  jmp_buf& getSetJmpBuffer() {return buffer;}
};


/// CircularBase - This class represents a circular list. Classes that extend
/// this class automatically place their instances in a circular list.
///
class CircularBase {
  /// _next - The next object in the list.
  ///
  CircularBase  *_next;

  /// _prev - The previous object in the list.
  ///
  CircularBase  *_prev;
public:
  
  /// ~CircularBase - Give the class a home.
  ///
  virtual ~CircularBase() {}

  /// next - Get the next object in the list.
  ///
  inline CircularBase *next() { return _next; }

  /// prev - Get the previous object in the list.
  ///
  inline CircularBase *prev() { return _prev; }

  /// next - Set the next object in the list.
  ///
  inline void next(CircularBase *n) { _next = n; }

  /// prev - Set the previous object in the list.
  ///
  inline void prev(CircularBase *p) { _prev = p; }

  /// CricularBase - Creates the object as a single element in the list.
  ///
  inline CircularBase() { alone(); }

  /// CircularBase - Creates the object and place it in the given list.
  ///
  inline explicit CircularBase(CircularBase *p) { append(p); }

  /// remove - Remove the object from its list.
  ///
  inline void remove() {
    _prev->_next = _next; 
    _next->_prev = _prev;
    alone();
  }

  /// append - Add the object in the list.
  ///
  inline void append(CircularBase *p) { 
    _prev = p;
    _next = p->_next;
    _next->_prev = this;
    _prev->_next = this;
  }

  /// alone - Set the object as being part of a new empty list.
  ///
  inline void alone() { _prev = _next = this; }

  /// print - Print the list for debug purposes.
  void print() {
    CircularBase* temp = this;
    do {
      fprintf(stderr, "%p -> ", (void*)temp);
      temp = temp->next();
    } while (temp != this);
    fprintf(stderr, "\n");
  }
};


class ExceptionBuffer;

/// Thread - This class is the base of custom virtual machines' Thread classes.
/// It provides static functions to manage threads. An instance of this class
/// contains all thread-specific informations.
class Thread : public CircularBase {
public:
  Thread() {
    lastExceptionBuffer = 0;
    lastKnownFrame = 0;
    runningDeadIsolate = false;
  }

  /// yield - Yield the processor to another thread.
  ///
  static void yield(void);
  
  /// kill - Kill the thread with the given pid by sending it a signal.
  ///
  static int kill(void* tid, int signo);
  
  /// kill - Kill the given thread by sending it a signal.
  ///
  int kill(int signo);
  
  /// exit - Exit the current thread.
  ///
  static void exit(int value);
  
  /// start - Start the execution of a thread.
  ///
  virtual int start(void (*fct)(vmkit::Thread*));
  
  uint64_t getThreadID() {
    return (uint64_t)this;
  }
 
protected:

  /// IsolateID - The Isolate ID of the thread's VM.
  isolate_id_t isolateID;

public:
  /// MyVM - The VM attached to this Thread.
  VirtualMachine* MyVM;

  /// baseSP - The base stack pointer.
  ///
  void* baseSP;
 
  /// doYield - Flag to tell the thread to yield for GC reasons.
  ///
  bool doYield;

  /// inRV - Flag to tell that the thread is being part of a rendezvous.
  ///
  bool inRV;

  /// joinedRV - Flag to tell that the thread has joined a rendezvous.
  ///
  bool joinedRV;

  /// get - Get the thread specific data of the current thread.
  ///
  static Thread* get() {
    return (Thread*)((word_t)StackWalker_getCallFrameAddress() & System::GetThreadIDMask());
  }
  
private:
  
  /// lastSP - If the thread is running native code that can not be
  /// interrupted, lastSP is not null and contains the value of the
  /// stack pointer before entering native.
  ///
  void* lastSP;
 
  /// internalThreadID - The implementation specific thread id.
  ///
  void* internalThreadID;
  
  /// internalThreadStart - The implementation specific thread starter
  /// function.
  ///
  static void internalThreadStart(vmkit::Thread* th);

  /// internalClearException - Clear any pending exception.
  ///
  virtual void internalClearException() {}

public:
 
  isolate_id_t getIsolateID() const;
  void setIsolateID(isolate_id_t newIsolateID);
  static isolate_id_t getValidIsolateID(isolate_id_t isolateID);

  bool runsDeadIsolate() const;
  void markRunningDeadIsolate();
  void setDeadIsolateID();

  bool isCurrentThread();

  /// tracer - Does nothing. Used for child classes which may defined
  /// a tracer.
  ///
  virtual void tracer(word_t closure) {}
  void scanStack(word_t closure);
  
  void* getLastSP() { return lastSP; }
  void  setLastSP(void* V) { lastSP = V; }
  
  void joinRVBeforeEnter();
  void joinRVAfterLeave(void* savedSP);

  void enterUncooperativeCode(uint16_t level = 0) __attribute__ ((noinline));
  void enterUncooperativeCode(void* SP);
  void leaveUncooperativeCode();
  void* waitOnSP();


  /// clearException - Clear any pending exception of the current thread.
  void clearException() {
    internalClearException();
  }

  bool isVmkitThread() const {
    if (!baseAddr) return false;
    else return (((word_t)this) & System::GetVmkitThreadMask()) == baseAddr;
  }

  /// baseAddr - The base address for all threads.
  static word_t baseAddr;

  /// OverflowMask - Apply this mask to implement overflow checks. For
  /// efficiency, we lower the available size of the stack: it can never go
  /// under 0xC0000
  ///
  static const uint64_t StackOverflowMask = 0xC0000;

  /// stackOverflow - Returns if there is a stack overflow in Java land.
  ///
  bool stackOverflow() {
    return ((word_t)StackWalker_getCallFrameAddress() & StackOverflowMask) == 0;
  }

  /// operator new - Allocate the Thread object as well as the stack for this
  /// Thread. The thread object is inlined in the stack.
  ///
  void* operator new(size_t sz);
  void operator delete(void* th) { UNREACHABLE(); }
  
  /// releaseThread - Free the stack so that another thread can use it.
  ///
  static void releaseThread(vmkit::Thread* th);

  /// routine - The function to invoke when the thread starts.
  ///
  void (*routine)(vmkit::Thread*);
 
  /// printBacktrace - Print the backtrace.
  ///
  void printBacktrace();
 
  /// getFrameContext - Fill the buffer with frames currently on the stack.
  ///
  void getFrameContext(void** buffer);
  
  /// getFrameContextLength - Get the length of the frame context.
  ///
  uint32_t getFrameContextLength();

  /// lastKnownFrame - The last frame that we know of, before resuming to JNI.
  ///
  KnownFrame* lastKnownFrame;
  
  /// lastExceptionBuffer - The last exception buffer on this thread's stack.
  ///
  ExceptionBuffer* lastExceptionBuffer;

  void internalThrowException();

  void startKnownFrame(KnownFrame& F) __attribute__ ((noinline));
  void endKnownFrame();
  void startUnknownFrame(KnownFrame& F) __attribute__ ((noinline));
  void endUnknownFrame();

  void* GetAlternativeStackEnd() {
    return (void*)((intptr_t)this + System::GetPageSize());
  }

  void* GetAlternativeStackStart() {
    return (void*)((intptr_t)GetAlternativeStackEnd() + System::GetAlternativeStackSize());
  }

  bool IsStackOverflowAddr(void* addr) {
    void* stackOverflowCheck = GetAlternativeStackStart();
    return (addr > stackOverflowCheck) &&
      addr <= (void*)((intptr_t)stackOverflowCheck + System::GetPageSize());
  }

  virtual void throwNullPointerException(void* methodIP) const;
  virtual void throwDeadIsolateException() const {}

  virtual void runAfterLeavingGarbageCollectorRendezVous() {}

protected:
  bool runningDeadIsolate;
};


} // end namespace vmkit
#endif // VMKIT_THREAD_H
