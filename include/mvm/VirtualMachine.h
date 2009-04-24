//===--------- VirtualMachine.h - Registering a VM ------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Ultimately, this would be like a generic way of defining a VM. But we're not
// quite there yet.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_VIRTUALMACHINE_H
#define MVM_VIRTUALMACHINE_H

#include "mvm/Allocator.h"
#include "mvm/CompilationUnit.h"
#include "mvm/Object.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include <cassert>
#include <map>


// Same values than JikesRVM
#define INITIAL_QUEUE_SIZE 256
#define GROW_FACTOR 2

#if (__WORDSIZE == 64)
#define LOG_BYTES_IN_ADDRESS 3
#else
#define LOG_BYTES_IN_ADDRESS 2
#endif

namespace jnjvm {
  class JavaCompiler;
  class JnjvmClassLoader;
}

namespace mvm {

/// VirtualMachine - This class is the root of virtual machine classes. It
/// defines what a VM should be.
///
class VirtualMachine : public mvm::PermanentObject {
protected:

  VirtualMachine() {
#ifdef SERVICE
    memoryLimit = ~0;
    executionLimit = ~0;
    GCLimit = ~0;
    threadLimit = ~0;
    parent = this;
    status = 1;
    _since_last_collection = 4*1024*1024;
#endif
    
    FinalizationQueue = new gc*[INITIAL_QUEUE_SIZE];
    QueueLength = INITIAL_QUEUE_SIZE;
    
    ToBeFinalized = new gc*[INITIAL_QUEUE_SIZE];
    ToBeFinalizedLength = INITIAL_QUEUE_SIZE;
  }
public:

  virtual void TRACER {}

  virtual ~VirtualMachine() {}

  /// runApplication - Run an application. The application name is in
  /// the arguments, hence it is the virtual machine's job to parse them.
  virtual void runApplication(int argc, char** argv) = 0;
  
  /// waitForExit - Wait until the virtual machine stops its execution.
  virtual void waitForExit() = 0;

  static jnjvm::JnjvmClassLoader* initialiseJVM(jnjvm::JavaCompiler* C,
                                                bool dlLoad = true);
  static VirtualMachine* createJVM(jnjvm::JnjvmClassLoader* C = 0);
  
  static CompilationUnit* initialiseCLIVM();
  static VirtualMachine* createCLIVM(CompilationUnit* C = 0);
 
protected:

  /// JavaFunctionMap - Map of Java method to function pointers. This map is
  /// used when walking the stack so that VMKit knows which Java method is
  /// executing on the stack.
  ///
  std::map<void*, void*> Functions;

  /// FunctionMapLock - Spin lock to protect the JavaFunctionMap.
  ///
  mvm::SpinLock FunctionMapLock;

public:
  /// addMethodInFunctionMap - A new method pointer in the function map.
  ///
  template <typename T>
  void addMethodInFunctionMap(T* meth, void* addr) {
    FunctionMapLock.acquire();
    Functions.insert(std::make_pair((void*)addr, meth));
    FunctionMapLock.release();
  }
  
  /// IPToJavaMethod - Map an instruction pointer to the Java method.
  ///
  template <typename T> T* IPToMethod(void* ip) {
    FunctionMapLock.acquire();
    std::map<void*, void*>::iterator I = Functions.upper_bound(ip);
    assert(I != Functions.begin() && "Wrong value in function map");
    FunctionMapLock.release();

    // Decrement because we had the "greater than" value.
    I--;
    return (T*)I->second;
  }

private:
  /// FinalizationQueueLock - A lock to protect access to the queue.
  ///
  mvm::SpinLock FinalizationQueueLock;

  /// finalizationQueue - A list of allocated objets that contain a finalize
  /// method.
  ///
  gc** FinalizationQueue;

  /// CurrentIndex - Current index in the queue of finalizable objects.
  ///
  uint32 CurrentIndex;

  /// QueueLength - Current length of the queue of finalizable objects.
  ///
  uint32 QueueLength;

  /// growQueue - Grow the queue of finalizable objects.
  ///
  void growQueue();
  
  /// ToBeFinalized - List of objects that are scheduled to be finalized.
  ///
  gc** ToBeFinalized;
  
  /// ToBeFinalizedLength - Current length of the queue of objects scheduled
  /// for finalization.
  ///
  uint32 ToBeFinalizedLength;

  /// CurrentFinalizedIndex - The current index in the ToBeFinalized queue
  /// that will be sceduled for finalization.
  ///
  uint32 CurrentFinalizedIndex;
  
  /// LastFinalizedIndex - The last index in the ToBeFinalized queue whose
  /// finalize method has been called.
  ///
  uint32 LastFinalizedIndex;
  
  /// finalizationCond - Condition variable to wake up finalization threads.
  ///
  mvm::Cond FinalizationCond;

  /// finalizationLock - Lock for the condition variable.
  ///
  mvm::LockNormal FinalizationLock;

  /// countFinalized - The number of entries to be finalized.
  ///
  uint32 countFinalized() {
    return (LastFinalizedIndex - CurrentFinalizedIndex + ToBeFinalizedLength) 
      % ToBeFinalizedLength;
  }

  /// freeFinalized - The number of entries available in the ToBeFinalized
  /// queue.
  ///
  uint32 freeFinalized() {
    return ToBeFinalizedLength - countFinalized();
  }

protected:
  /// invokeFinalizer - Invoke the finalizer of the object. This may involve
  /// changing the environment, e.g. going to native to Java.
  ///
  virtual void invokeFinalizer(gc*) {}


public:
  /// finalizerStart - The start function of a finalizer. Will poll the
  /// finalizationQueue.
  ///
  static void finalizerStart(mvm::Thread*);

  /// addFinalizationCandidate - Add an object to the queue of objects with
  /// a finalization method.
  ///
  void addFinalizationCandidate(gc*);
  
  /// scanFinalizationQueue - Scan objets with a finalized method and schedule
  /// them for finalization if they are not live.
  ///
  void scanFinalizationQueue();

  /// wakeUpFinalizers - Wake the finalizers.
  ///
  void wakeUpFinalizers() { FinalizationCond.broadcast(); }

#ifdef ISOLATE
  size_t IsolateID;
#endif

#ifdef SERVICE
  uint64_t memoryUsed;
  uint64_t gcTriggered;
  uint64_t executionTime;
  uint64_t numThreads;
  CompilationUnit* CU;
  virtual void stopService() {}

  uint64_t memoryLimit;
  uint64_t executionLimit;
  uint64_t threadLimit;
  uint64_t GCLimit;

  int _since_last_collection;
  VirtualMachine* parent;
  uint32 status; 
#endif

  mvm::Allocator gcAllocator;

};


} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
