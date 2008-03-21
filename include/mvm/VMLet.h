//===--------------- VMLet.h - Definition of VMLets -----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_VMLET_H
#define MVM_VMLET_H

#include "mvm/Object.h"
#include "mvm/Threads/Key.h"

namespace llvm {
  class Module;
  class ExistingModuleProvider;
  class ExecutionEngine;
  class FunctionPassManager;
  class JITMemoryManager;
}

namespace mvm {

class VMLet : public Object {
public:
  static VirtualTable *VT;
  //GC_defass(String, name);

  static void register_sigsegv_handler(void (*fct)(int, void *));

  static void initialise();
};

} // end namespace mvm

#endif // MVM_VMLET_H
