//===---------------- JavaIsolate.h - Isolates ----------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_ISOLATE_H
#define JNJVM_JAVA_ISOLATE_H


#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#include "Jnjvm.h"

namespace jnjvm {

class ArrayObject;
class JavaThread;
class JavaObject;
class ThreadSystem;

class ThreadSystem {
public:
  uint16 nonDaemonThreads;
  mvm::Lock* nonDaemonLock;
  mvm::Cond* nonDaemonVar;
  
  ThreadSystem() {
    nonDaemonThreads = 1;
    nonDaemonLock = mvm::Lock::allocNormal();
    nonDaemonVar  = mvm::Cond::allocCond();
  }

  ~ThreadSystem() {
    delete nonDaemonLock;
    delete nonDaemonVar;
  }

};

class JavaIsolate : public Jnjvm {
public:
  static VirtualTable* VT;
  ThreadSystem* threadSystem;
  JavaThread* bootstrapThread;
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  ~JavaIsolate();
  JavaIsolate();

  JavaObject* loadAppClassLoader();
  void loadBootstrap();
  void executeClass(const char* className, ArrayObject* args);
  void executePremain(const char* className, JavaString* args,
                      JavaObject* instrumenter);
  void waitForExit();
  void runMain(int argc, char** argv);
  void mapInitialThread();
  static void runIsolate(const char* className, ArrayObject* args);
  static JavaIsolate* allocateIsolate(Jnjvm* callingVM);
  static JavaIsolate* allocateBootstrap();
  
};

} // end namespace jnjvm

#endif
