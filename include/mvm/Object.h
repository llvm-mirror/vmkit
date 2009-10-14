//===---------- Object.h - Common layout for all objects ------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_OBJECT_H
#define MVM_OBJECT_H

#include "types.h"
#include "MvmGC.h"

namespace mvm {


#define VT_DESTRUCTOR_OFFSET 0
#define VT_OPERATOR_DELETE_OFFSET 1
#define VT_TRACER_OFFSET 2
#define VT_PRINT_OFFSET 3
#define VT_HASHCODE_OFFSET 4
#define VT_NB_FUNCS 5
#define VT_SIZE 5 * sizeof(void*)


class PrintBuffer;

/// Object - Root of all objects. Define default implementations of virtual
/// methods and some commodity functions.
///
class Object : public gc {
public:
	static void     default_tracer(gc *o)                    {}
	static intptr_t default_hashCode(const gc *o)            { return (intptr_t)o; }
	static void     default_print(const gc *o, PrintBuffer *buf);

  /// tracer - Default implementation of tracer. Does nothing.
  ///
  virtual void tracer() { default_tracer(this); }

  /// print - Default implementation of print.
  ///
  virtual void      print(PrintBuffer *buf) const { default_print(this, buf); }

  /// hashCode - Default implementation of hashCode. Returns the address
  /// of this object.
  ///
  virtual intptr_t  hashCode() const { return default_hashCode(this);}
  
};

} // end namespace mvm

#endif // MVM_OBJECT_H
