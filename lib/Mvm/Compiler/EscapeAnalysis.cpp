//===------EscapeAnalysis.cpp - Simple LLVM escape analysis ---------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "llvm/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

#include <map>
#include "unistd.h"

#include "mvm/GC/GC.h"

using namespace llvm;

namespace {

  class VISIBILITY_HIDDEN EscapeAnalysis : public FunctionPass {
  public:
    static char ID;
    uint64_t pageSize;
    EscapeAnalysis(Function* alloc = 0) : 
      FunctionPass((intptr_t)&ID) {
      Allocator = alloc;
      pageSize = getpagesize();
    }

    virtual bool runOnFunction(Function &F);
  private:
    Function* Allocator;
    bool processMalloc(Instruction* I, Value* Size, Value* VT);
  };
  char EscapeAnalysis::ID = 0;
  RegisterPass<EscapeAnalysis> X("EscapeAnalysis", "Escape Analysis Pass");

bool EscapeAnalysis::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 

    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        if (CI->getOperand(0) == Allocator) {
          Changed |= processMalloc(CI, CI->getOperand(1), CI->getOperand(2));
        }
      } else if (InvokeInst *CI = dyn_cast<InvokeInst>(I)) {
        if (CI->getOperand(0) == Allocator) {
          Changed |= processMalloc(CI, CI->getOperand(3), CI->getOperand(4));
        }
      }
    }
  }
  return Changed;
}




static bool escapes(Instruction* Ins, std::map<Instruction*, bool>& visited) {
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
        if (!visited[II]) {
          visited[II] = true;
          if (escapes(II, visited)) return true;
        }
      }
    } else {
      return true;
    }
  }
  return false;
}

bool EscapeAnalysis::processMalloc(Instruction* I, Value* Size, Value* VT) {
  Instruction* Alloc = I;
  
  ConstantExpr* CE = dyn_cast<ConstantExpr>(VT);
  if (CE) {
    ConstantInt* C = dyn_cast<ConstantInt>(CE->getOperand(0));
    if (!C) return false;

    VirtualTable* Table = (VirtualTable*)C->getZExtValue();
    ConstantInt* CI = dyn_cast<ConstantInt>(Size);
    // If the class has a finalize method, do not stack allocate the object.
    if (!((void**)Table)[0] && CI) {
      std::map<Instruction*, bool> visited;
      uint64_t NSize = CI->getZExtValue();
      if (NSize < pageSize && !(escapes(Alloc, visited))) {
        AllocaInst* AI = new AllocaInst(Type::Int8Ty, Size, "", Alloc);
        BitCastInst* BI = new BitCastInst(AI, Alloc->getType(), "", Alloc);
        DOUT << "escape" << Alloc->getParent()->getParent()->getName() << "\n";
        Alloc->replaceAllUsesWith(BI);
        // If it's an invoke, replace the invoke with a direct branch.
        if (InvokeInst *CI = dyn_cast<InvokeInst>(Alloc)) {
          BranchInst::Create(CI->getNormalDest(), Alloc);
        }
        Alloc->eraseFromParent();
        return true;
      }
    }
  }
  return false;
}
}

namespace mvm {
FunctionPass* createEscapeAnalysisPass(llvm::Function* alloc) {

  return new EscapeAnalysis(alloc);
}
}
