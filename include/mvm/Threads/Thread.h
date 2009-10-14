//===---------------- Threads.h - Micro-vm threads ------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_THREAD_H
#define MVM_THREAD_H

#include <cassert>
#include <stdlib.h>
#include <vector>

#include "types.h"

namespace mvm {

class VirtualMachine;

/// CircularBase - This class represents a circular list. Classes that extend
/// this class automatically place their instances in a circular list.
///
class CircularBase {
  /// _next - The next object in the list.
  ///
	CircularBase	*_next;

  /// _prev - The previous object in the list.
  ///
	CircularBase	*_prev;
public:

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


/// Thread - This class is the base of custom virtual machines' Thread classes.
/// It provides static functions to manage threads. An instance of this class
/// contains all thread-specific informations.
class Thread : public CircularBase {
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

  /// inGC - Flag to tell that the thread is being part of a GC.
  ///
  bool inGC;

  /// stackScanned - Flag to tell that the thread's stack has already
  /// been analyzed.
  ///
  bool stackScanned;

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

  /// internalClearException - Clear any pending exception.
  ///
  virtual void internalClearException() {}

  /// joinCollection - Join a collection.
  ///
  void joinCollection();

public:
 
  /// ~Thread - Give the class a home.
  ///
  virtual ~Thread() {}

  /// tracer - Does nothing. Used for child classes which may defined
  /// a tracer.
  ///
  virtual void tracer() {}
  
  void* getLastSP() { return lastSP; }
  void  setLastSP(void* V) { lastSP = V; }

  void enterUncooperativeCode(unsigned level = 0) __attribute__ ((noinline)) {
    if (isMvmThread()) {
      if (!inGC) {
        assert(!lastSP && "SP already set when entering uncooperative code");
        ++level;
        void* temp = __builtin_frame_address(0);
        while (level--) temp = ((void**)temp)[0];
        lastSP = temp;
        if (doYield) joinCollection();
        assert(lastSP && "No last SP when entering uncooperative code");
      }
    }
  }
  
  void enterUncooperativeCode(void* SP) {
    if (isMvmThread()) {
      if (!inGC) {
        assert(!lastSP && "SP already set when entering uncooperative code");
        lastSP = SP;
        if (doYield) joinCollection();
        assert(lastSP && "No last SP when entering uncooperative code");
      }
    }
  }

  void leaveUncooperativeCode() {
    if (isMvmThread()) {
      if (!inGC) {
        assert(lastSP && "No last SP when leaving uncooperative code");
        if (doYield) joinCollection();
        lastSP = 0;
        if (doYield) joinCollection();
        assert(!lastSP && "SP has a value after leaving uncooperative code");
      }
    }
  }


  /// clearException - Clear any pending exception of the current thread.
  void clearException() {
    internalClearException();
  }

  bool isMvmThread() {
    if (!baseAddr) return false;
    else return (((uintptr_t)this) & MvmThreadMask) == baseAddr;
  }

  /// baseAddr - The base address for all threads.
  static uintptr_t baseAddr;

  /// IDMask - Apply this mask to the stack pointer to get the Thread object.
  ///
  static const uint64_t IDMask = 0x7FF00000;

  /// MvmThreadMask - Apply this mask to verify that the current thread was
  /// created by Mvm.
  ///
  static const uint64_t MvmThreadMask = 0xF0000000;

  /// OverflowMask - Apply this mask to implement overflow checks. For
  /// efficiency, we lower the available size of the stack: it can never go
  /// under 0xC0000
  ///
  static const uint64_t StackOverflowMask = 0xC0000;

  /// operator new - Allocate the Thread object as well as the stack for this
  /// Thread. The thread object is inlined in the stack.
  ///
  void* operator new(size_t sz);
  
  /// operator delete - Free the stack so that another thread can use it.
  ///
  void operator delete(void* obj);

  /// routine - The function to invoke when the thread starts.
  ///
  void (*routine)(mvm::Thread*);
 
#ifdef SERVICE
  /// stoppingService - The service that is currently stopping.
  ///
  VirtualMachine* stoppingService;  
#endif

  /// getBacktrace - Return the back trace of this thread.
  ///
  int getBacktrace(void** stack, int size) {
    void** addr = (void**)__builtin_frame_address(0);
    int cpt = 0;
    while (addr && cpt < size && addr < baseSP && addr < addr[0]) {
      addr = (void**)addr[0];
      stack[cpt++] = (void**)FRAME_IP(addr);
    }
    return cpt;
  }

  /// printBacktraceAfterSignal - Print the backtrace during a signal
  /// handler.
  ///
  virtual void printBacktraceAfterSignal() {}

  /// addresses - The list of return addresses which represent native/app cross
  /// calls.
  ///
  std::vector<void*> addresses;

  /// startNative - Record that we are entering native code.
  ///
  void startNative(int level) __attribute__ ((noinline));

  /// endNative - Record that we are leaving native code.
  ///
  void endNative() {
    assert(!(addresses.size() % 2) && "Wrong stack");    
    addresses.pop_back();
  }

};


} // end namespace mvm
#endif // MVM_THREAD_H
