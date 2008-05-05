//===----- LowerConstantCalls.cpp - Changes arrayLength calls  --------------===//
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

  class VISIBILITY_HIDDEN LowerConstantCalls : public FunctionPass {
  public:
    static char ID;
    LowerConstantCalls() : FunctionPass((intptr_t)&ID) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerConstantCalls::ID = 0;
  RegisterPass<LowerConstantCalls> X("LowerArrayLength", "Lower Array length");

bool LowerConstantCalls::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        Value* V = CI->getOperand(0);
        if (V == jnjvm::JavaJIT::arrayLengthLLVM) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the array
          Value* array = new BitCastInst(val, jnjvm::JavaArray::llvmType, "", CI);
          std::vector<Value*> args; //size=  2
          args.push_back(mvm::jit::constantZero);
          args.push_back(jnjvm::JavaArray::sizeOffset());
          Value* ptr = GetElementPtrInst::Create(array, args.begin(), args.end(),
                                         "", CI);
          Value* load = new LoadInst(ptr, "", CI);
          CI->replaceAllUsesWith(load);
          CI->eraseFromParent();
        } else if (V == jnjvm::JavaJIT::getVTLLVM) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the object
          std::vector<Value*> indexes; //[3];
          indexes.push_back(mvm::jit::constantZero);
          indexes.push_back(mvm::jit::constantZero);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                           indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == jnjvm::JavaJIT::getClassLLVM) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the object
          std::vector<Value*> args2;
          args2.push_back(mvm::jit::constantZero);
          args2.push_back(jnjvm::JavaObject::classOffset());
          Value* classPtr = GetElementPtrInst::Create(val, args2.begin(),
                                                      args2.end(), "",
                                                      CI);
          Value* cl = new LoadInst(classPtr, "", CI);
          CI->replaceAllUsesWith(cl);
          CI->eraseFromParent();
        }
      }
    }
  }
  return Changed;
}


FunctionPass* createLowerConstantCallsPass() {
  return new LowerConstantCalls();
}
}
