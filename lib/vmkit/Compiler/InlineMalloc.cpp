//===------------- InlineMalloc.cpp - Inline allocations  -----------------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "vmkit/JIT.h"

using namespace llvm;

namespace vmkit {

  class InlineMalloc : public FunctionPass {
  public:
    static char ID;
    InlineMalloc() : FunctionPass(ID) {}
    virtual const char* getPassName() const {
      return "Inline malloc and barriers";
    }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char InlineMalloc::ID = 0;


bool InlineMalloc::runOnFunction(Function& F) {
  Function* VTMalloc = F.getParent()->getFunction("VTgcmalloc");
  Function* vmkitMalloc = F.getParent()->getFunction("vmkitgcmalloc");
  Function* FieldWriteBarrier = F.getParent()->getFunction("fieldWriteBarrier");
  Function* ArrayWriteBarrier = F.getParent()->getFunction("arrayWriteBarrier");
  Function* NonHeapWriteBarrier = F.getParent()->getFunction("nonHeapWriteBarrier");
  bool Changed = false;
  const DataLayout *DL = getAnalysisIfAvailable<DataLayout>();
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (I->getOpcode() != Instruction::Call &&
          I->getOpcode() != Instruction::Invoke) {
        continue;
      }
      CallSite Call(I);
      Function* Temp = Call.getCalledFunction();
      if (Temp == VTMalloc ||
      		Temp == vmkitMalloc) {
        if (dyn_cast<Constant>(Call.getArgument(0))) {
          InlineFunctionInfo IFI(NULL, DL);
          Changed |= InlineFunction(Call, IFI);
          break;
        }
      } else if (Temp == FieldWriteBarrier ||
                 Temp == NonHeapWriteBarrier ||
                 Temp == ArrayWriteBarrier) {
        InlineFunctionInfo IFI(NULL, DL);
        Changed |= InlineFunction(Call, IFI);
        break;
      }
    }
  }
  return Changed;
}


FunctionPass* createInlineMallocPass() {
  return new InlineMalloc();
}

}
