//===------- LowerArrayLength.cpp - Lowers array length calls -------------===//
//
//                           N3
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

#include "VMArray.h"
#include "CLIJit.h"

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
  static RegisterPass<LowerArrayLength> X("LowerArrayLength", "Lower Array length");

bool LowerArrayLength::runOnFunction(Function& F) {
  bool Changed = false;
   for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 

    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE; ) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        if (CI->getOperand(0) == n3::CLIJit::arrayLengthLLVM) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the array
          std::vector<Value*> args; //size=  2
          args.push_back(ConstantInt::get(Type::Int32Ty, 0));
          args.push_back(n3::VMArray::sizeOffset());
          Value* ptr = GetElementPtrInst::Create(val, args.begin(), args.end(),
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

LowerArrayLength* createLowerArrayLengthPass() {
  return new LowerArrayLength();
}

}

