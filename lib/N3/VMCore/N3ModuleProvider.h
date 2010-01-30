//===-------- N3ModuleProvider.h - LLVM Module Provider for N3 ------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_MODULE_PROVIDER_H
#define N3_MODULE_PROVIDER_H

#include <llvm/GVMaterializer.h>
#include <llvm/Module.h>

#include "LockedMap.h"

using namespace llvm;

namespace n3 {

class N3ModuleProvider : public GVMaterializer {
public:
  FunctionMap* functions;
  Module* TheModule;

  N3ModuleProvider(Module *m, FunctionMap* f) {
    TheModule = m;
    m->setMaterializer(this);
    functions = f;
  }
  
  bool Materialize(GlobalValue *GV, std::string *ErrInfo = 0);
  virtual bool isMaterializable(const llvm::GlobalValue*) const;
  virtual bool isDematerializable(const llvm::GlobalValue*) const {
    return false;
  }
  virtual bool MaterializeModule(llvm::Module*, std::string*) { return true; }

  VMMethod* lookupFunction(Function* F) {
    return functions->lookup(F);
  }
};

} // End n3 namespace

#endif
