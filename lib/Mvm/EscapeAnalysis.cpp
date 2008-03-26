//===------EscapeAnalysis.cpp - Simple LLVM escape analysis ---------------===//
//
//                     The Micro Virtual Machine
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

#include <map>

using namespace llvm;

namespace {

  class VISIBILITY_HIDDEN EscapeAnalysis : public FunctionPass {
  public:
    static char ID;
    EscapeAnalysis(Function* alloc = 0, Function* init = 0) : 
      FunctionPass((intptr_t)&ID) {
      Allocator = alloc;
      Initialize = init;
    }

    virtual bool runOnFunction(Function &F);
  private:
    Function* Allocator;
    Function* Initialize;
    bool processMalloc(Instruction* I);
  };
  char EscapeAnalysis::ID = 0;
  RegisterPass<EscapeAnalysis> X("EscapeAnalysis", "Escape Analysis Pass");
}

bool EscapeAnalysis::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 

    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE; 
         II++) {
      Instruction *I = II;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        if (CI->getOperand(0) == Allocator) {
          Changed |= processMalloc(CI);
        }
      } else if (InvokeInst *CI = dyn_cast<InvokeInst>(I)) {
        if (CI->getOperand(0) == Allocator) {
          Changed |= processMalloc(CI);
        }
      }
    }
  }
  return Changed;
}

namespace mvm {

EscapeAnalysis* createEscapeAnalysisPass(Function* alloc, Function* init) {
  return new EscapeAnalysis(alloc, init);
}

}

static bool escapes(Instruction* Ins, std::map<AllocaInst*, bool>& visited) {
  for (Value::use_iterator I = Ins->use_begin(), E = Ins->use_end(); 
       I != E; ++I) {
    if (Instruction* II = dyn_cast<Instruction>(I)) {
      if (dyn_cast<CallInst>(II)) return true;
      else if (dyn_cast<InvokeInst>(II)) return true;
      else if (dyn_cast<BitCastInst>(II)) {
        if (escapes(II, visited)) return true;
      }
      else if (StoreInst* SI = dyn_cast<StoreInst>(II)) {
        if (AllocaInst * AI = dyn_cast<AllocaInst>(SI->getOperand(1))) {
          if (!visited[AI]) {
            visited[AI] = true;
            if (escapes(AI, visited)) return true;
          }
        } else if (SI->getOperand(0) == Ins) {
          return true;
        }
      }
      else if (dyn_cast<LoadInst>(II)) {
        if (isa<PointerType>(II->getType())) {
          if (escapes(II, visited)) return true; // allocas
        }
      }
      else if (dyn_cast<GetElementPtrInst>(II)) {
        if (escapes(II, visited)) return true;
      }
      else if (dyn_cast<ReturnInst>(II)) return true;
      else if (dyn_cast<PHINode>(II)) {
        if (escapes(II, visited)) return true;
      }
    } else {
      return true;
    }
  }
  return false;
}

bool EscapeAnalysis::processMalloc(Instruction* I) {
  Instruction* Alloc = I;
  Value::use_iterator UI = I->use_begin(), UE = I->use_end(), Last = I->use_begin();
  while (UI != UE) { Last = UI; UI++;}
  
  if (BitCastInst *BCI = dyn_cast<BitCastInst>(Last)) {
    I = BCI;
  } 
  
  std::map<AllocaInst*, bool> visited;
  if (!(escapes(Alloc, visited))) {
    AllocaInst* AI = new AllocaInst(I->getType()->getContainedType(0), "", Alloc);
    BitCastInst* BI = new BitCastInst(AI, Alloc->getType(), "", Alloc);
    std::vector<Value*> Args;
    if (isa<CallInst>(Alloc)) {
      Args.push_back(Alloc->getOperand(1));
    } else {
      Args.push_back(Alloc->getOperand(3)); // skip unwind and normal BB
    }
    Args.push_back(BI);
    Instruction* CI;
    if (isa<CallInst>(Alloc)) {
      CI = new CallInst(Initialize, Args.begin(), Args.end(), "", Alloc);
    } else {
      CI = new InvokeInst(Initialize, ((InvokeInst*)Alloc)->getNormalDest(), 
                          ((InvokeInst*)Alloc)->getUnwindDest(), Args.begin(), 
                          Args.end(), "", Alloc->getParent());
    }
    DOUT << "escape" << Alloc->getParent()->getParent()->getName() << "\n";
    Alloc->replaceAllUsesWith(CI);
    Alloc->eraseFromParent();
    return true;
  }
  return false;
}
