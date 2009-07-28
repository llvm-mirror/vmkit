//===------- LoopSafePoints.cpp - Add safe points in loop headers ---------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

  class VISIBILITY_HIDDEN LoopSafePoints : public LoopPass {
  public:
    static char ID;
    
    LoopSafePoints() : LoopPass((intptr_t)&ID) {}

    virtual bool runOnLoop(Loop* L, LPPassManager& LPM);

  private:
    void insertSafePoint(BasicBlock* BB, Function* SafeFunction,
                         Value* YieldPtr);
  };

  char LoopSafePoints::ID = 0;
  RegisterPass<LoopSafePoints> X("LoopSafePoints",
                                 "Add safe points in loop headers");

void LoopSafePoints::insertSafePoint(BasicBlock* BB, Function* SafeFunction,
                                     Value* YieldPtr) {
  Instruction* I = BB->getFirstNonPHI();
  BasicBlock* NBB = BB->splitBasicBlock(I);

  NBB = NBB->getSinglePredecessor();
  I = NBB->getTerminator();
  BasicBlock* SU = (static_cast<BranchInst*>(I))->getSuccessor(0);
  I->eraseFromParent();
  
  Value* Ld = new LoadInst(YieldPtr, "", NBB);
  BasicBlock* yield = BasicBlock::Create("", BB->getParent());
  BranchInst::Create(yield, SU, Ld, NBB);

  CallInst::Create(SafeFunction, "", yield);
  BranchInst::Create(SU, yield);
}

bool LoopSafePoints::runOnLoop(Loop* L, LPPassManager& LPM) {
 
  BasicBlock* Header = L->getHeader();
  Function *F = Header->getParent();  
  Function* SafeFunction =
    F->getParent()->getFunction("conditionalSafePoint");
  if (!SafeFunction) return false;

  Value* YieldPtr = 0;
  
  // Lookup the yield pointer.
  for (Function::iterator BI = F->begin(), BE = F->end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI;

    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      CallSite Call = CallSite::get(I);
      if (Call.getInstruction() && Call.getCalledValue() == SafeFunction) {
        if (BasicBlock* Incoming = Cur->getSinglePredecessor()) {
          if (BranchInst* T = dyn_cast<BranchInst>(Incoming->getTerminator())) {
            if (LoadInst* LI = dyn_cast<LoadInst>(T->getCondition())) {
              YieldPtr = LI->getPointerOperand();
              break;
            }
          }
        }
      }
    }
    if (YieldPtr) break;
  }

  assert(YieldPtr && "Could not find initial yield pointer.");

  TerminatorInst* TI = Header->getTerminator();
  
  // Insert the check after the entry block if the entry block does the
  // loop exit.
  if (BranchInst* BI = dyn_cast<BranchInst>(TI)) {
    if (BI->isConditional()) {

      BasicBlock* First = BI->getSuccessor(0);
      BasicBlock* Second = BI->getSuccessor(1);

      bool containsFirst = L->contains(First);
      bool containsSecond = L->contains(Second);

      if (!containsFirst) {
        insertSafePoint(Second, SafeFunction, YieldPtr);
        return true;
      }
      
      if (!containsSecond) {
        insertSafePoint(First, SafeFunction, YieldPtr);
        return true;
      }
    }
  }
  
  insertSafePoint(Header, SafeFunction, YieldPtr);
  return true;
}

}


namespace mvm {

LoopPass* createLoopSafePointsPass() {
  return new LoopSafePoints();
}

}
