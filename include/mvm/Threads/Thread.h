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


class Collector;

namespace mvm {

class VirtualMachine;

class CircularBase {
	CircularBase	*_next;
	CircularBase	*_prev;
public:
	inline CircularBase *next() { return _next; }
	inline CircularBase *prev() { return _prev; }

	inline void next(CircularBase *n) { _next = n; }
	inline void prev(CircularBase *p) { _prev = p; }

	inline CircularBase() { alone(); }
	inline explicit CircularBase(CircularBase *p) { append(p); }

	inline void remove() { 
		_prev->_next = _next; 
		_next->_prev = _prev;
		alone();
	}

	inline void append(CircularBase *p) { 
		_prev = p;
		_next = p->_next;
		_next->_prev = this;
		_prev->_next = this;
	}

	inline void alone() { _prev = _next = this; }
};


#if defined(__MACH__) && !defined(__i386__)
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
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

#ifdef ISOLATE
  /// IsolateID - The Isolate ID of the thread's VM.
  size_t IsolateID;
#endif

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
  virtual void internalClearException() {}

public:
  
  virtual ~Thread() {}
  virtual void TRACER {}


  /// clearException - Clear any pending exception of the current thread.
  static void clearException() {
    Thread* th = Thread::get();
    th->internalClearException();
  }

  static const uint64_t IDMask = 0x7FF00000;

  void* operator new(size_t sz);
  
  void operator delete(void* obj);

  void (*routine)(mvm::Thread*);
 
#ifdef SERVICE
  VirtualMachine* stoppingService;  
#endif

};


} // end namespace mvm
#endif // MVM_THREAD_H
