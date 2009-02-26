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

#include "types.h"

#include "MvmGC.h"


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



#if defined(__MACH__) && defined(__PPC__)
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
  
  /// yield - Yield the processor to another thread. If the thread has been
  /// askink for yield already a number of times (n), then do a small sleep.
  ///
  static void yield(unsigned int* n);
  
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
  int start(void (*fct)(mvm::Thread*));
  
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
  
  /// get - Get the thread specific data of the current thread.
  ///
  static Thread* get() {
    return (Thread*)((uintptr_t)__builtin_frame_address(0) & IDMask);
  }
  
private:
  
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

public:
 
  /// ~Thread - Give the class a home.
  ///
  virtual ~Thread() {}

  /// tracer - Does nothing. Used for child classes which may defined
  /// a tracer.
  ///
  virtual void TRACER {}


  /// clearException - Clear any pending exception of the current thread.
  void clearException() {
    internalClearException();
  }

  /// IDMask - Apply this mask to the stack pointer to get the Thread object.
  ///
  static const uint64_t IDMask = 0x7FF00000;

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

};


} // end namespace mvm
#endif // MVM_THREAD_H
