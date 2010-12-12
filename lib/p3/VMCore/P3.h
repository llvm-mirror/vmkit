#ifndef _P3_H_
#define _P3_H_

#include "mvm/VirtualMachine.h"

namespace p3 {

class P3Reader;

class P3 : public mvm::VirtualMachine {
public:
  /// P3 - default constructor
  ///
	P3(mvm::BumpPtrAllocator& alloc, mvm::VMKit* vmkit);


  /// finalizeObject - invoke the finalizer of an object
  ///
	virtual void finalizeObject(mvm::gc* obj);

  /// getReferentPtr - return the referent of a reference
  ///
	virtual mvm::gc** getReferent(mvm::gc* ref);

  /// setReferentPtr - set the referent of a reference
  ///
	virtual void setReferent(mvm::gc* ref, mvm::gc* val);

  /// enqueueReference - enqueue the reference
  ///
	virtual bool enqueueReference(mvm::gc* _obj);

  /// tracer - Trace this virtual machine's GC-objects. 
	///    Called once by vm. If you have GC-objects in a thread specific data, redefine the tracer of your VMThreadData.
  ///
  virtual void tracer(uintptr_t closure);

  /// getObjectSize - Get the size of this object. Used by copying collectors.
  ///
  virtual size_t getObjectSize(mvm::gc* object);

  /// getObjectTypeName - Get the type of this object. Used by the GC for
  /// debugging purposes.
  ///
  virtual const char* getObjectTypeName(mvm::gc* object);

	/// runApplicationImpl - code executed after a runApplication in a vmkit thread
	///
	virtual void runApplicationImpl(int argc, char** argv);

public:
	/// runFile - execute the file fileName
	///
	void runFile(const char* fileName);
};

} // namespace p3

#endif
