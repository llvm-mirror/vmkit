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

class JnjvmModule;

class JnjvmModuleProvider : public ModuleProvider {
private:
  JavaMethod* staticLookup(Class* caller, uint32 index);

public:
  FunctionMap* functions;
  FunctionDefMap* functionDefs;
  JnjvmModuleProvider(JnjvmModule *m, FunctionMap* fm, FunctionDefMap* fdm) {
    TheModule = (Module*)m;
    functions = fm;
    functionDefs= fdm;
  }
  

  bool materializeFunction(Function *F, std::string *ErrInfo = 0);
  void* materializeFunction(JavaMethod* meth); 

  Module* materializeModule(std::string *ErrInfo = 0) { return TheModule; }
};

} // End jnjvm namespace

#endif
