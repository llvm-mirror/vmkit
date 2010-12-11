#ifndef _VMKIT_H_
#define _VMKIT_H_

#include "mvm/Allocator.h"
#include "mvm/Threads/CollectionRV.h"
#include "mvm/VirtualMachine.h"

#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class Module;
  class TargetMachine;
}

namespace mvm {
class MethodInfo;
class VMKit;
class gc;
class FinalizerThread;
class ReferenceThread;

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

  /// removeMethodInfos - Remove all MethodInfo owned by the given owner.
  void removeMethodInfos(void* owner);

  FunctionMap();
};


class NonDaemonThreadManager {
	friend class Thread;
public:
	NonDaemonThreadManager() { nonDaemonThreads = 0; }

private:
  /// nonDaemonThreads - Number of threads in the system that are not daemon
  /// threads.
  //
  uint16 nonDaemonThreads;

  /// nonDaemonLock - Protection lock for the nonDaemonThreads variable.
  ///
  mvm::LockNormal nonDaemonLock;

  /// nonDaemonVar - Condition variable to wake up the initial thread when it
  /// waits for other non-daemon threads to end. The non-daemon thread that
  /// decrements the nonDaemonThreads variable to zero wakes up the initial
  /// thread.
  ///
  mvm::Cond nonDaemonVar;
  
  /// leave - A thread calls this function when it leaves the thread system.
  ///
  void leaveNonDaemonMode();

  /// enter - A thread calls this function when it enters the thread system.
  ///
  void enterNonDaemonMode();

public:
	void waitNonDaemonThreads();
};

class VMKit : public mvm::PermanentObject {
public:
  /// allocator - Bump pointer allocator to allocate permanent memory of VMKit
  mvm::BumpPtrAllocator& allocator;

	// initialise - initialise vmkit. If never called, will be called by the first constructor of vmkit
	static void initialise(llvm::CodeGenOpt::Level = llvm::CodeGenOpt::Default,
                         llvm::Module* TheModule = 0,
                         llvm::TargetMachine* TheTarget = 0);

  VMKit(mvm::BumpPtrAllocator &Alloc);

	LockRecursive                _vmkitLock;

	void vmkitLock() { _vmkitLock.lock(); }
	void vmkitUnlock() { _vmkitLock.unlock(); }

	/// ------------------------------------------------- ///
	/// ---             vm managment                  --- ///
	/// ------------------------------------------------- ///
	// vms - the list of vms. 
	//       synchronized with vmkitLock
	VirtualMachine**             vms;
	size_t                       vmsArraySize;

	size_t addVM(VirtualMachine* vm);
	void   removeVM(size_t id);

	/// ------------------------------------------------- ///
	/// ---             thread managment              --- ///
	/// ------------------------------------------------- ///
  /// preparedThreads - the list of prepared threads, they are not yet running.
	///                   synchronized with vmkitLock
  ///
	CircularBase<Thread>         preparedThreads;

  /// runningThreads - the list of running threads
	///                  synchronize with vmkitLock
  ///
	CircularBase<Thread>         runningThreads;

  /// numberOfRunningThreads - The number of threads that currently run under this VM.
	///                          synchronized with vmkitLock
	///
  size_t                       numberOfRunningThreads;

	// nonDaemonThreadsManager - manager of the non daemon threads
	NonDaemonThreadManager       nonDaemonThreadsManager;

	void registerPreparedThread(mvm::Thread* th);  
	void unregisterPreparedThread(mvm::Thread* th);

	void registerRunningThread(mvm::Thread* th);  
	void unregisterRunningThread(mvm::Thread* th);

	void waitNonDaemonThreads();

	/// ------------------------------------------------- ///
	/// ---             memory managment              --- ///
	/// ------------------------------------------------- ///
  /// rendezvous - The rendezvous implementation for garbage collection.
  ///
#ifdef WITH_LLVM_GCC
  CooperativeCollectionRV      rendezvous;
#else
  UncooperativeCollectionRV    rendezvous;
#endif

private:
  /// enqueueThread - The thread that finalizes references.
  ///
	FinalizerThread*             finalizerThread;
  
  /// enqueueThread - The thread that enqueues references.
  ///
  ReferenceThread*             referenceThread;

  /// getAndAllocateFinalizerThread - get the finalizer thread and allocate it if it does not exist
  ///
	FinalizerThread*             getAndAllocateFinalizerThread();

  /// getAndAllocateReferenceThread - get the reference thread and allocate it if it does not exist
  ///
	ReferenceThread*             getAndAllocateReferenceThread();

public:
  /// addWeakReference - Add a weak reference to the queue.
  ///
  void addWeakReference(mvm::gc* ref);
  
  /// addSoftReference - Add a weak reference to the queue.
  ///
  void addSoftReference(mvm::gc* ref);
  
  /// addPhantomReference - Add a weak reference to the queue.
  ///
  void addPhantomReference(mvm::gc* ref);

  /// addFinalizationCandidate - Add an object to the queue of objects with
  /// a finalization method.
  ///
  void addFinalizationCandidate(gc* object);

  /// scanFinalizationQueue - Scan objets with a finalized method and schedule
  /// them for finalization if they are not live.
  /// 
  void scanFinalizationQueue(uintptr_t closure);
  
  /// scanWeakReferencesQueue - Scan all weak references. Called by the GC
  /// before scanning the finalization queue.
  /// 
  void scanWeakReferencesQueue(uintptr_t closure);
  
  /// scanSoftReferencesQueue - Scan all soft references. Called by the GC
  /// before scanning the finalization queue.
  ///
  void scanSoftReferencesQueue(uintptr_t closure);
  
  /// scanPhantomReferencesQueue - Scan all phantom references. Called by the GC
  /// after the finalization queue.
  ///
  void scanPhantomReferencesQueue(uintptr_t closure);

	bool startCollection(); // 1 ok, begin collection, 0 do not start collection
	void endCollection();

  void tracer(uintptr_t closure);

	/// ------------------------------------------------- ///
	/// ---    backtrace related methods              --- ///
	/// ------------------------------------------------- ///
  /// FunctionsCache - cache of compiled functions
	//  
  FunctionMap FunctionsCache;

  MethodInfo* IPToMethodInfo(void* ip) {
    return FunctionsCache.IPToMethodInfo(ip);
  }

  void removeMethodInfos(void* owner) {
    FunctionsCache.removeMethodInfos(owner);
  }
};

}

#endif
