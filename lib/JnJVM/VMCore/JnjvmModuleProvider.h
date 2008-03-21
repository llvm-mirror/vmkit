//===----- JnjvmModuleProvider.h - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_MODULE_PROVIDER_H
#define JNJVM_MODULE_PROVIDER_H

#include <llvm/ModuleProvider.h>

#include "LockedMap.h"

using namespace llvm;

namespace jnjvm {

class JnjvmModuleProvider : public ModuleProvider {
public:
  FunctionMap* functions;
  JnjvmModuleProvider(Module *m, FunctionMap* fm) {
    TheModule = m;
    functions = fm;
  }
  
  bool materializeFunction(Function *F, std::string *ErrInfo = 0);

  Module* materializeModule(std::string *ErrInfo = 0) { return TheModule; }
};

} // End jnjvm namespace

#endif
