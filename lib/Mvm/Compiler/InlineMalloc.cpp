//===------------- InlineMalloc.cpp - Inline allocations  -----------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "mvm/JIT.h"

using namespace llvm;

namespace mvm {

  class InlineMalloc : public FunctionPass {
  public:
    static char ID;
    InlineMalloc() : FunctionPass((intptr_t)&ID) {}

    virtual bool runOnFunction(Function &F);
  private:
  };
  char InlineMalloc::ID = 0;

#if 0
  static RegisterPass<InlineMalloc> X("InlineMalloc",
                                      "Inline calls to gcmalloc");
#endif


bool InlineMalloc::runOnFunction(Function& F) {
  Function* Malloc = mvm::MvmModule::globalModule->getFunction("gcmalloc");
  if (Malloc->isDeclaration()) return false;
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      CallSite Call = CallSite::get(I);
      Instruction* CI = Call.getInstruction();
      if (CI) {
        Function* F = Call.getCalledFunction();
        if (F && F->getName() == "gcmalloc") {
          if (dyn_cast<Constant>(Call.getArgument(0))) {
            Call.setCalledFunction(mvm::MvmModule::globalModule->getFunction("gcmalloc"));
            Changed |= InlineFunction(Call, 0, mvm::MvmModule::TheTargetData);
            break;
          }
        }
      }
    }
  }
  return Changed;
}


FunctionPass* createInlineMallocPass() {
  return new InlineMalloc();
}

}
