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
	virtual VMThreadData* buildVMThreadData(Thread* mut) { return new VMThreadData(this, mut); }

	/// runApplicationImpl - code executed after a runApplication in a vmkit thread
	///
	virtual void runApplicationImpl(int argc, char** argv) {}

	/// runApplication - launch runApplicationImpl in a vmkit thread. The vmData is not allocated.
	///
	void runApplication(int argc, char** argv);

	/// runApplication - launch starter in a vmkit thread. The vmData is not allocated.
	///
	void runApplication(void (*starter)(VirtualMachine* vm, int argc, char** argv), int argc, char** argv);
  
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
