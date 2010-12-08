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
#include "mvm/Threads/CollectionRV.h"
#include "mvm/Threads/Locks.h"

#include <cassert>
#include <map>

namespace mvm {

/// VirtualMachine - This class is the root of virtual machine classes. It
/// defines what a VM should be.
///
class VirtualMachine : public mvm::PermanentObject {
private:
	friend class VMKit;
	VirtualMachine(mvm::BumpPtrAllocator &Alloc) : allocator(Alloc) {}

protected:
  VirtualMachine(mvm::BumpPtrAllocator &Alloc, mvm::VMKit* vmk);

public:
  virtual ~VirtualMachine();

  /// allocator - Bump pointer allocator to allocate permanent memory
  /// related to this VM.
  ///
  mvm::BumpPtrAllocator& allocator;

	/// vmkit - a pointer to vmkit that contains information on all the vms
	///
	mvm::VMKit* vmkit;

	/// vmID - id of the vm
	size_t vmID;

//===----------------------------------------------------------------------===//
// (1) thread-related methods.
//===----------------------------------------------------------------------===//
	/// buildVMThreadData - allocate a java thread for the underlying mutator. Called when the java thread is a foreign thread.
	///
	virtual VMThreadData* buildVMThreadData(Thread* mut) = 0; //{ return new VMThreadData(this, mut); }

  /// nbNonDaemonThreads - Number of threads in the system that are not daemon
  /// threads.
  uint16 nbNonDaemonThreads;

  /// nonDaemonLock - Protection lock for the nonDaemonThreads variable.
  mvm::LockNormal nonDaemonLock;

  /// nonDaemonVar - Condition variable to wake up the initial thread when it
  /// waits for other non-daemon threads to end. The non-daemon thread that
  /// decrements the nonDaemonThreads variable to zero wakes up the initial
  /// thread.
  mvm::Cond nonDaemonVar;
  
  /// enter - A thread calls this function when it enters the thread system.
  virtual void enterNonDeamonMode();

  /// leave - A thread calls this function when it leaves the thread system.
  virtual void leaveNonDeamonMode();

	virtual void runApplicationImpl(int argc, char** argv) {};

  /// runInNonDeamonThread - start a non deamon thread a begin the code with start if != 0 and runApplicationImpl otherwise
	/// the thread leaves the deamon mode when it finishs.
	void runInNonDeamonThread(void (*start)(VirtualMachine*, int, char**), int argc, char** argv);

  /// runInNonDeamonThread - start a non deamon thread a begin the code with runApplicationImpl, 
	void runInNonDeamonThread(int argc, char **argv) { runInNonDeamonThread(0, argc, argv); }

  /// waitNonDeamonThreads - wait until all the non deamon threads are terminated.
	void waitForNonDeamonThreads();

  /// runApplication - Run an application. The application name is in
  /// the arguments, hence it is the virtual machine's job to parse them.
  virtual void runApplication(int argc, char** argv) = 0;
  
  /// waitForExit - Wait until the virtual machine stops its execution.
  virtual void waitForExit() = 0;

//===----------------------------------------------------------------------===//
// (2) GC-related methods.
//===----------------------------------------------------------------------===//
  /// startCollection - Preliminary code before starting a GC.
  ///
  virtual void startCollection() {}
  
  /// endCollection - Code after running a GC.
  ///
  virtual void endCollection() {}

  /// finalizeObject - invoke the finalizer of a java object
  ///
	virtual void finalizeObject(mvm::gc* obj) {}

  /// getReferentPtr - return the referent of a reference
  ///
	virtual mvm::gc** getReferent(mvm::gc* ref) = 0;

  /// setReferentPtr - set the referent of a reference
  ///
	virtual void setReferent(mvm::gc* ref, mvm::gc* val) = 0;

  /// enqueueReference - enqueue the reference
  ///
	virtual bool enqueueReference(mvm::gc* _obj) = 0;

  /// tracer - Trace this virtual machine's GC-objects. 
	///    Called once by vm. If you have GC-objects in a thread specific data, redefine the tracer of your VMThreadData.
  ///
  virtual void tracer(uintptr_t closure) {}

  /// getObjectSize - Get the size of this object. Used by copying collectors.
  ///
  virtual size_t getObjectSize(gc* object) = 0;

  /// getObjectTypeName - Get the type of this object. Used by the GC for
  /// debugging purposes.
  ///
  virtual const char* getObjectTypeName(gc* object) { return "An object"; }

//===----------------------------------------------------------------------===//
// (4) Launch-related methods.
//===----------------------------------------------------------------------===//
};

} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
