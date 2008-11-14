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

#include <llvm/ModuleProvider.h>

#include "LockedMap.h"

using namespace llvm;

namespace n3 {

class N3ModuleProvider : public ModuleProvider {
public:
  FunctionMap* functions;
  N3ModuleProvider(Module *m, FunctionMap* f) {
    TheModule = m;
    functions = f;
  }
  
  bool materializeFunction(Function *F, std::string *ErrInfo = 0);

  Module* materializeModule(std::string *ErrInfo = 0) { return TheModule; }

  VMMethod* lookupFunction(Function* F) {
    return functions->lookup(F);
  }
};

} // End n3 namespace

#endif
