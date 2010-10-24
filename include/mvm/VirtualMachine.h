//===--------- VirtualMachine.h - Registering a VM ------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_VIRTUALMACHINE_H
#define MVM_VIRTUALMACHINE_H

#include "mvm/Allocator.h"
#include "mvm/MethodInfo.h"
#include "mvm/Threads/CollectionRV.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/GC/GC.h"

#include <cassert>
#include <map>

namespace j3 {
  class JavaCompiler;
  class JnjvmClassLoader;
}

namespace mvm {

class FunctionMap {
public:
  /// Functions - Map of applicative methods to function pointers. This map is
  /// used when walking the stack so that VMKit knows which applicative method
  /// is executing on the stack.
  ///
  std::map<void*, MethodInfo*> Functions;

  /// FunctionMapLock - Spin lock to protect the Functions map.
  ///
  mvm::SpinLock FunctionMapLock;

  /// IPToMethodInfo - Map a code start instruction instruction to the MethodInfo.
  ///
  MethodInfo* IPToMethodInfo(void* ip);

  /// addMethodInfo - A new instruction pointer in the function map.
  ///
  void addMethodInfo(MethodInfo* meth, void* ip);

  FunctionMap();
};

/// VirtualMachine - This class is the root of virtual machine classes. It
/// defines what a VM should be.
///
class VirtualMachine : public mvm::PermanentObject {
protected:
  VirtualMachine(mvm::BumpPtrAllocator &Alloc) :
		  allocator(Alloc) {
    mainThread = 0;
    NumberOfThreads = 0;
  }

  virtual ~VirtualMachine() {
    if (scanner) delete scanner;
  }

public:

  /// allocator - Bump pointer allocator to allocate permanent memory
  /// related to this VM.
  ///
  mvm::BumpPtrAllocator& allocator;

//===----------------------------------------------------------------------===//
// (1) Thread-related methods.
//===----------------------------------------------------------------------===//

  /// mainThread - The main thread of this VM.
  ///
  mvm::Thread* mainThread;

  /// NumberOfThreads - The number of threads that currently run under this VM.
  ///
  uint32_t NumberOfThreads;

  /// ThreadLock - Lock to create or destroy a new thread.
  ///
  mvm::SpinLock ThreadLock;
  
  /// setMainThread - Set the main thread of this VM.
  ///
  void setMainThread(mvm::Thread* th) { mainThread = th; }
  
  /// getMainThread - Get the main thread of this VM.
  ///
  mvm::Thread* getMainThread() const { return mainThread; }

  /// addThread - Add a new thread to the list of threads.
  ///
  void addThread(mvm::Thread* th) {
    ThreadLock.lock();
    NumberOfThreads++;
    if (th != mainThread) {
      if (mainThread) th->append(mainThread);
      else mainThread = th;
    }
    ThreadLock.unlock();
  }
  
  /// removeThread - Remove the thread from the list of threads.
  ///
  void removeThread(mvm::Thread* th) {
    ThreadLock.lock();
    NumberOfThreads--;
    if (mainThread == th) mainThread = (Thread*)th->next();
    th->remove();
    if (!NumberOfThreads) mainThread = 0;
    ThreadLock.unlock();
  }


//===----------------------------------------------------------------------===//
// (2) GC-related methods.
//===----------------------------------------------------------------------===//

  /// startCollection - Preliminary code before starting a GC.
  ///
  virtual void startCollection() {}
  
  /// endCollection - Code after running a GC.
  ///
  virtual void endCollection() {}
  
  /// scanWeakReferencesQueue - Scan all weak references. Called by the GC
  /// before scanning the finalization queue.
  /// 
  virtual void scanWeakReferencesQueue(uintptr_t closure) {}
  
  /// scanSoftReferencesQueue - Scan all soft references. Called by the GC
  /// before scanning the finalization queue.
  ///
  virtual void scanSoftReferencesQueue(uintptr_t closure) {}
  
  /// scanPhantomReferencesQueue - Scan all phantom references. Called by the GC
  /// after the finalization queue.
  ///
  virtual void scanPhantomReferencesQueue(uintptr_t closure) {}

  /// scanFinalizationQueue - Scan objets with a finalized method and schedule
  /// them for finalization if they are not live.
  /// 
  virtual void scanFinalizationQueue(uintptr_t closure) {}

  /// addFinalizationCandidate - Add an object to the queue of objects with
  /// a finalization method.
  ///
  virtual void addFinalizationCandidate(gc* object) {}

  /// tracer - Trace this virtual machine's GC-objects.
  ///
  virtual void tracer(uintptr_t closure) {}

  /// scanner - Scanner of threads' stacks.
  ///
  mvm::StackScanner* scanner;

  mvm::StackScanner* getScanner() {
    return scanner;
  }

  /// rendezvous - The rendezvous implementation for garbage collection.
  ///
#ifdef WITH_LLVM_GCC
  CooperativeCollectionRV rendezvous;
#else
  UncooperativeCollectionRV rendezvous;
#endif

//===----------------------------------------------------------------------===//
// (3) Backtrace-related methods.
//===----------------------------------------------------------------------===//

  FunctionMap FunctionsCache;
  MethodInfo* IPToMethodInfo(void* ip) {
    return FunctionsCache.IPToMethodInfo(ip);
  }
  
//===----------------------------------------------------------------------===//
// (4) Launch-related methods.
//===----------------------------------------------------------------------===//

  /// runApplication - Run an application. The application name is in
  /// the arguments, hence it is the virtual machine's job to parse them.
  virtual void runApplication(int argc, char** argv) = 0;
  
  /// waitForExit - Wait until the virtual machine stops its execution.
  virtual void waitForExit() = 0;
};

} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
