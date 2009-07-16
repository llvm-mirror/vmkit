//===-- LowerJavaRT.cpp - Remove references to RT classes and functions  --===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

#include "jnjvm/JnjvmModule.h"

#include <iostream>

using namespace llvm;

namespace {

  class VISIBILITY_HIDDEN LowerJavaRT : public ModulePass {
  public:
    static char ID;
    LowerJavaRT() : ModulePass((intptr_t)&ID) { }

    virtual bool runOnModule(Module &M);
  private:
  };
  char LowerJavaRT::ID = 0;
  static RegisterPass<LowerJavaRT> X("LowerJavaRT",
                                     "Remove references to RT");

bool LowerJavaRT::runOnModule(Module& M) {
  bool Changed = true;

  for (Module::iterator I = M.begin(), E = M.end(); I != E;) {
    GlobalValue& GV = *I;
    ++I;
    if (!strncmp(GV.getName().c_str(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().c_str(), "java", 4)) {
      GV.replaceAllUsesWith(M.getContext().getNullValue(GV.getType()));
      GV.eraseFromParent();
    }
  }

  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E;) {
    GlobalValue& GV = *I;
    ++I;
    if (!strncmp(GV.getName().c_str(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().c_str(), "java", 4)) {
      GV.replaceAllUsesWith(M.getContext().getNullValue(GV.getType()));
      GV.eraseFromParent();
    }
  }


  return Changed;
}


ModulePass* createLowerJavaRT() {
  return new LowerJavaRT();
}

}
