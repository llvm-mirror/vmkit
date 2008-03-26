//===----- LowerArrayLength.cpp - Changes arrayLength calls  --------------===//
//
//                               JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaJIT.h"

using namespace llvm;

namespace mvm {

  class VISIBILITY_HIDDEN LowerArrayLength : public FunctionPass {
  public:
    static char ID;
    LowerArrayLength() : FunctionPass((intptr_t)&ID) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerArrayLength::ID = 0;
  RegisterPass<LowerArrayLength> X("LowerArrayLength", "Lower Array length");

bool LowerArrayLength::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 

    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE; 
         II++) {
      Instruction *I = II;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        if (CI->getOperand(0) == jnjvm::JavaJIT::arrayLengthLLVM) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the array
          Value* array = new BitCastInst(val, jnjvm::JavaArray::llvmType, "", CI);
          std::vector<Value*> args; //size=  2
          args.push_back(mvm::jit::constantZero);
          args.push_back(jnjvm::JavaArray::sizeOffset());
          Value* ptr = new GetElementPtrInst(array, args.begin(), args.end(),
                                         "", CI);
          Value* load = new LoadInst(ptr, "", CI);
          CI->replaceAllUsesWith(load);
          CI->eraseFromParent();
        }
      }
    }
  }
  return Changed;
}


FunctionPass* createLowerArrayLengthPass() {
  return new LowerArrayLength();
}
}
