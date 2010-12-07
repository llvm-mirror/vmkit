//===---------------- Threads.h - Micro-vm threads ------------------------===//
//
//                        The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_THREAD_H
#define MVM_THREAD_H

#include <cassert>
#include <cstdio>
#include <stdlib.h>

#include "debug.h"
#include "types.h"

#ifdef RUNTIME_DWARF_EXCEPTIONS
  #define TRY try
  #define CATCH catch(...)
  #define IGNORE catch(...) { mvm::Thread::get()->clearPendingException(); }
  #define END_CATCH
#else
  #include <csetjmp>
  #if defined(__MACH__)
    #define TRY { mvm::ExceptionBuffer __buffer__; if (!_setjmp(__buffer__.buffer))
  #else
    #define TRY { mvm::ExceptionBuffer __buffer__; if (!setjmp(__buffer__.buffer))
  #endif
  #define CATCH else
  #define IGNORE else { mvm::Thread::get()->clearPendingException(); }}
  #define END_CATCH }
#endif

namespace mvm {

class gc;
class MethodInfo;
class VirtualMachine;

/// CircularBase - This class represents a circular list. Classes that extend
/// this class automatically place their instances in a circular list.
///
// WARNING: if you modify this class, you must also change mvm-runtime.ll
template<typename T>
class CircularBase {
  /// _next - The next object in the list.
  ///
  T  *_next;

  /// _prev - The previous object in the list.
  ///
  T  *_prev;
public:
  /// ~CircularBase - Give the class a home.
  ///
  virtual ~CircularBase() {}

  /// next - Get the next object in the list.
  ///
  inline T *next() { return _next; }

  /// prev - Get the previous object in the list.
  ///
  inline T *prev() { return _prev; }

  /// CricularBase - Creates the object as a single element in the list.
  ///
  inline CircularBase() { _prev = _next = (T*)this; }

  /// CircularBase - Creates the object and place it in the given list.
  ///
  inline explicit CircularBase(CircularBase<T> *p) { appendTo(p); }

  /// remove - Remove the object from its list.
  ///
  inline void remove() {
    _prev->_next = _next; 
    _next->_prev = _prev;
    _prev = _next = (T*)this;
  }

  /// append - Add the object in the list.
  ///
  inline void appendTo(CircularBase<T> *p) { 
    _prev = (T*)p;
    _next = p->_next;
    _next->_prev = (T*)this;
    _prev->_next = (T*)this;
  }

  /// print - Print the list for debug purposes.
  void print() {
    T* temp = (T*)this;
    do {
      fprintf(stderr, "%p -> ", (void*)temp);
      temp = temp->next0();
    } while (temp != this);
    fprintf(stderr, "\n");
  }
};



#if defined(__MACH__) && (defined(__PPC__) || defined(__ppc__))
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
#endif

// Apparently gcc for i386 and family considers __builtin_frame_address(0) to
// return the caller, not the current function.
#if defined(__i386__) || defined(i386) || defined(_M_IX86) || \
    defined(__x86_64__) || defined(_M_AMD64)
#define FRAME_PTR() __builtin_frame_address(0)
#else
#define FRAME_PTR() (((void**)__builtin_frame_address(0))[0])
#endif


class KnownFrame {
public:
  void* currentFP;
  void* currentIP;
  KnownFrame* previousFrame;
};


class ExceptionBuffer;
class Thread;
class VMKit;

// WARNING: if you modify this class, you must also change mvm-runtime.ll
// WARNING: when a VMThreadData is store in a thread (in allVmsData), you must never delete it yourself.
class VMThreadData {
public:
  /// mut - The associated thread mutator
	Thread*         mut;

  /// vm - The associated virtual machine
	VirtualMachine* vm;

	VMThreadData(VirtualMachine *v, Thread* m) {
		this->mut = m;
		this->vm = v;
	}

  virtual void tracer(uintptr_t closure) {};

	virtual ~VMThreadData() {} // force the construction of a VT
};

/// Thread - This class is the base of custom virtual machines' Thread classes.
/// It provides static functions to manage threads. An instance of this class
/// contains all thread-specific informations.
// WARNING: if you modify this class, you must also change mvm-runtime.ll
class Thread : public CircularBase<Thread> {
protected:
  Thread(VMKit* vmk);

public:

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
  virtual int start(void (*fct)(mvm::Thread*));
  
  uint64_t getThreadID() {
    return (uint64_t)this;
  }
 
public:

  /// IsolateID - The Isolate ID of the thread's VM.
  size_t IsolateID;

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
    return (Thread*)((uintptr_t)__builtin_frame_address(0) & IDMask);
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
  
  /// internalThreadStart - The implementation sepcific thread starter
  /// function.
  ///
  static void internalThreadStart(mvm::Thread* th);

public:
 
  /// tracer - trace the pendingException and the vmData
  ///
  virtual void tracer(uintptr_t closure);

  void scanStack(uintptr_t closure);
  
  void* getLastSP() { return lastSP; }
  void  setLastSP(void* V) { lastSP = V; }
  
  void joinRVBeforeEnter();
  void joinRVAfterLeave(void* savedSP);

  void enterUncooperativeCode(unsigned level = 0) __attribute__ ((noinline));
  void enterUncooperativeCode(void* SP);
  void leaveUncooperativeCode();
  void* waitOnSP();


  /// clearPendingException - Clear any pending exception of the current thread.
  void clearPendingException() {
		pendingException = 0;
#ifdef RUNTIME_DWARF_EXCEPTIONS
    internalPendingException = 0;
#endif
  }

  /// setException - only set the pending exception
	///
	Thread* setPendingException(gc *obj);

  /// throwIt - Throw a pending exception.
  ///
  void throwIt();

  /// getPendingException - Return the pending exception.
  ///
  gc* getPendingException() {
    return pendingException;
  }

  bool isMvmThread() {
    if (!baseAddr) return false;
    else return (((uintptr_t)this) & MvmThreadMask) == baseAddr;
  }

  /// baseAddr - The base address for all threads.
  static uintptr_t baseAddr;

  /// IDMask - Apply this mask to the stack pointer to get the Thread object.
  ///
#if (__WORDSIZE == 64)
  static const uint64_t IDMask = 0xF7FF00000;
#else
  static const uint64_t IDMask = 0x7FF00000;
#endif
  /// MvmThreadMask - Apply this mask to verify that the current thread was
  /// created by Mvm.
  ///
  static const uint64_t MvmThreadMask = 0xF0000000;

  /// OverflowMask - Apply this mask to implement overflow checks. For
  /// efficiency, we lower the available size of the stack: it can never go
  /// under 0xC0000
  ///
  static const uint64_t StackOverflowMask = 0xC0000;

  /// stackOverflow - Returns if there is a stack overflow in Java land.
  ///
  bool stackOverflow() {
    return ((uintptr_t)__builtin_frame_address(0) & StackOverflowMask) == 0;
  }

  /// operator new - Allocate the Thread object as well as the stack for this
  /// Thread. The thread object is inlined in the stack.
  ///
  void* operator new(size_t sz);
  void operator delete(void* th);
  virtual ~Thread();

  /// routine - The function to invoke when the thread starts.
  ///
  void (*routine)(mvm::Thread*);
 
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
  
#ifdef RUNTIME_DWARF_EXCEPTIONS
  void* internalPendingException;
#else
  /// lastExceptionBuffer - The last exception buffer on this thread's stack.
  ///
  ExceptionBuffer* lastExceptionBuffer;
#endif

  void startKnownFrame(KnownFrame& F) __attribute__ ((noinline));
  void endKnownFrame();
  void startUnknownFrame(KnownFrame& F) __attribute__ ((noinline));
  void endUnknownFrame();

  /// vmData - vm specific data
  ///
	VMThreadData* vmData;

  /// pendingException - the pending exception
  ///
	gc* pendingException;

public:
	/// vmkit - a (shortcut) pointer to vmkit that contains information on all the vms
	///
	mvm::VMKit* vmkit;

  /// allVmsData - the array of thread specific data.
  ///
	VMThreadData** allVmsData;

  /// reallocAllVmsData - realloc the allVmsData from old to n or 0 to n if allVmsData=0
	///   must be protected by rendezvous.threadLock
  ///
	void reallocAllVmsData(int old, int n);

	/// attach - attach the vm specific data of the given virtual machine
	///
	void attach(VirtualMachine* vm);
};

#ifndef RUNTIME_DWARF_EXCEPTIONS
class ExceptionBuffer {
public:
  ExceptionBuffer() {
    Thread* th = Thread::get();
    previousBuffer = th->lastExceptionBuffer;
    th->lastExceptionBuffer = this;
  }

  ~ExceptionBuffer() {
    Thread* th = Thread::get();
    assert(th->lastExceptionBuffer == this && "Wrong exception buffer");
    th->lastExceptionBuffer = previousBuffer;
  }
  ExceptionBuffer* previousBuffer;
  jmp_buf buffer;
};
#endif

/// StackWalker - This class walks the stack of threads, returning a MethodInfo
/// object at each iteration.
///
class StackWalker {
public:
  void** addr;
  void*  ip;
  KnownFrame* frame;
  mvm::Thread* thread;

  StackWalker(mvm::Thread* th) __attribute__ ((noinline));
  void operator++();
  void* operator*();
  MethodInfo* get();

};


} // end namespace mvm
#endif // MVM_THREAD_H
