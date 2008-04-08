//===----------- JavaThread.h - Java thread description -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_THREAD_H
#define JNJVM_JAVA_THREAD_H

#include <setjmp.h>

#include "llvm/PassManager.h"

#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Key.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

namespace jnjvm {

class Class;
class JavaObject;
class Jnjvm;

class JavaThread : public mvm::Thread {
public:
  static VirtualTable *VT;
  JavaObject* javaThread;
  Jnjvm* isolate;
  mvm::Lock* lock;
  mvm::Cond* varcond;
  JavaObject* pendingException;
  void* internalPendingException;
  unsigned int self;
  unsigned int interruptFlag;
  unsigned int state;
  llvm::FunctionPassManager* perFunctionPasses;
  std::vector<jmp_buf*> sjlj_buffers;

  static const unsigned int StateRunning;
  static const unsigned int StateWaiting;
  static const unsigned int StateInterrupted;

  JavaObject* cacheObject; // cache for allocations patching

  virtual void print(mvm::PrintBuffer *buf) const;
  virtual void tracer(size_t sz);
  virtual void destroyer(size_t sz);
  
  void initialise(JavaObject* thread, Jnjvm* isolate);
  static JavaThread* get();
  static JavaObject* currentThread();
  
  static void* getException();
  static void throwException(JavaObject*);
  static void throwPendingException();
  static void clearException();
  static bool compareException(Class*);
  static JavaObject* getJavaException();
  void returnFromNative();
};

} // end namespace jnjvm

#endif
