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

  /// getObjectSize - Get the size of this object. Used by copying collectors.
  ///
  virtual size_t getObjectSize(gc* object) { abort(); }

  /// getObjectTypeName - Get the type of this object. Used by the GC for
  /// debugging purposes.
  ///
  virtual const char* getObjectTypeName(gc* object) { return "An object"; }

//===----------------------------------------------------------------------===//
// (4) Launch-related methods.
//===----------------------------------------------------------------------===//

  /// runApplication - Run an application. The application name is in
  /// the arguments, hence it is the virtual machine's job to parse them.
  virtual void runApplication(int argc, char** argv) { abort(); }
  
  /// waitForExit - Wait until the virtual machine stops its execution.
  virtual void waitForExit() { abort(); }
};

} // end namespace mvm
#endif // MVM_VIRTUALMACHINE_H
