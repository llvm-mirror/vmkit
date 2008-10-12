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
#include "mvm/Allocator.h"
#include "mvm/JIT.h"
#include "mvm/Threads/Key.h"


class Collector;

namespace mvm {

/// Thread - This class is the base of custom virtual machines' Thread classes.
/// It provides static functions to manage threads. An instance of this class
/// contains all thread-specific informations.
class Thread : public gc {
public:
  
  /// yield - Yield the processor to another thread.
  ///
  static void yield(void);
  
  /// yield - Yield the processor to another thread. If the thread has been
  /// askink for yield already a number of times (n), then do a small sleep.
  ///
  static void yield(unsigned int* n);
  
  /// self - The thread id of the current thread, which is specific to the
  /// underlying implementation.
  ///
  static int self(void);

  /// initialise - Initialise the thread implementation. Used for pthread_key.
  ///
  static void initialise(void);

  /// kill - Kill the thread with the given pid by sending it a signal.
  ///
  static int kill(int tid, int signo);

  /// exit - Exit the current thread.
  ///
  static void exit(int value);
  
  /// start - Start the execution of a thread, creating it and setting its
  /// Thread instance.
  ///
  static int start(int *tid, int (*fct)(void *), void *arg);

private:
  /// threadKey - the key for accessing the thread specific data.
  ///
  static mvm::Key<Thread>* threadKey;
 
public:
  Allocator* allocator;
  
  /// baseSP - The base stack pointer.
  ///
  void* baseSP;
  
  /// threadID - The virtual machine specific thread id.
  ///
  uint32 threadID;

  /// get - Get the thread specific data of the current thread.
  ///
  static Thread* get() {
#if 1//defined(__PPC__) || defined(__ppc__)
    return (Thread*)Thread::threadKey->get();
#else
    return (Thread*)mvm::jit::getExecutionEnvironment();
#endif
  }
  
  /// set - Set the thread specific data of the current thread.
  ///
  static void set(Thread* th) {
#if 1//defined(__PPC__) || defined(__ppc__)
    Thread::threadKey->set(th);
#else
    mvm::jit::setExecutionEnvironment(th);
#endif
  }

private:
  
  /// internalClearException - Clear any pending exception.
  virtual void internalClearException() {
  }

public:

  /// clearException - Clear any pending exception of the current thread.
  static void clearException() {
    Thread* th = Thread::get();
    th->internalClearException();
  }
};


} // end namespace mvm
#endif // MVM_THREAD_H
