//===---- CompilingUnit.h - A compilation unit to compile source files ----===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// A compilation unit contains a module and a module provider to compile source
// files of a virtual machine, e.g .java files in Java.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_COMPILATION_UNIT_H
#define MVM_COMPILATION_UNIT_H

#include "mvm/Object.h"

namespace llvm {
  class FunctionPassManager;
  class Module;
  class ModuleProvider;
}

namespace mvm {

class MvmModule;

class CompilationUnit : public mvm::Object {
public:
  MvmModule* TheModule;
  llvm::ModuleProvider* TheModuleProvider;

  void AddStandardCompilePasses();
};
}


#endif
