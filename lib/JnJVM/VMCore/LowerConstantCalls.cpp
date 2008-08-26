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

#include "JnjvmModule.h"

using namespace llvm;
using namespace jnjvm;

namespace mvm {

  class VISIBILITY_HIDDEN LowerConstantCalls : public FunctionPass {
  public:
    static char ID;
    LowerConstantCalls() : FunctionPass((intptr_t)&ID) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerConstantCalls::ID = 0;
  static RegisterPass<LowerConstantCalls> X("LowerConstantCalls",
                                            "Lower Constant calls");

bool LowerConstantCalls::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        Value* V = CI->getOperand(0);
        if (V == jnjvm::JnjvmModule::ArrayLengthFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the array
          Value* array = new BitCastInst(val, jnjvm::JnjvmModule::JavaArrayType,
                                         "", CI);
          std::vector<Value*> args; //size=  2
          args.push_back(mvm::jit::constantZero);
          args.push_back(jnjvm::JnjvmModule::JavaArraySizeOffsetConstant);
          Value* ptr = GetElementPtrInst::Create(array, args.begin(), args.end(),
                                         "", CI);
          Value* load = new LoadInst(ptr, "", CI);
          CI->replaceAllUsesWith(load);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetVTFunction) {
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
        } else if (V == jnjvm::JnjvmModule::GetClassFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); // get the object
          std::vector<Value*> args2;
          args2.push_back(mvm::jit::constantZero);
          args2.push_back(jnjvm::JnjvmModule::JavaObjectClassOffsetConstant);
          Value* classPtr = GetElementPtrInst::Create(val, args2.begin(),
                                                      args2.end(), "",
                                                      CI);
          Value* cl = new LoadInst(classPtr, "", CI);
          CI->replaceAllUsesWith(cl);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetVTFromClassFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::jit::constantZero);
          indexes.push_back(jnjvm::JnjvmModule::OffsetVTInClassConstant);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetObjectSizeFromClassFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::jit::constantZero);
          indexes.push_back(JnjvmModule::OffsetObjectSizeInClassConstant);
          Value* SizePtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* Size = new LoadInst(SizePtr, "", CI);
          CI->replaceAllUsesWith(Size);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetDepthFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::jit::constantZero);
          indexes.push_back(JnjvmModule::OffsetDepthInClassConstant);
          Value* DepthPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                      indexes.end(), "", CI);
          Value* Depth = new LoadInst(DepthPtr, "", CI);
          CI->replaceAllUsesWith(Depth);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetDisplayFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::jit::constantZero);
          indexes.push_back(JnjvmModule::OffsetDisplayInClassConstant);
          Value* DisplayPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                        indexes.end(), "", CI);
          Value* Display = new LoadInst(DisplayPtr, "", CI);
          CI->replaceAllUsesWith(Display);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::GetClassInDisplayFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          Value* depth = CI->getOperand(2); 
          Value* ClassPtr = GetElementPtrInst::Create(val, depth, "", CI);
          Value* Class = new LoadInst(ClassPtr, "", CI);
          CI->replaceAllUsesWith(Class);
          CI->eraseFromParent();
        } else if (V == jnjvm::JnjvmModule::InstanceOfFunction) {
          ConstantExpr* CE = dyn_cast<ConstantExpr>(CI->getOperand(2));
          if (CE) {
            ConstantInt* C = (ConstantInt*)CE->getOperand(0);
            CommonClass* cl = (CommonClass*)C->getZExtValue();
            Changed = true;
            BasicBlock* NBB = II->getParent()->splitBasicBlock(II);
            I->getParent()->getTerminator()->eraseFromParent();
            Value* obj = CI->getOperand(1);
            Instruction* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj,
                                            JnjvmModule::JavaObjectNullConstant,
                                            "", CI);
            BasicBlock* ifTrue = BasicBlock::Create("", &F);
            BasicBlock* ifFalse = BasicBlock::Create("", &F);
            BranchInst::Create(ifTrue, ifFalse, cmp, CI);
            PHINode* node = PHINode::Create(Type::Int1Ty, "", ifTrue);
            node->addIncoming(ConstantInt::getFalse(), CI->getParent());
            Value* objCl = CallInst::Create(JnjvmModule::GetClassFunction, obj,
                                            "", ifFalse);
            
            if (isInterface(cl->access)) {
              std::vector<Value*> args;
              args.push_back(objCl);
              args.push_back(CE);
              Value* res = CallInst::Create(JnjvmModule::ImplementsFunction,
                                            args.begin(), args.end(), "",
                                            ifFalse);
              node->addIncoming(res, ifFalse);
              BranchInst::Create(ifTrue, ifFalse);
            } else {
              cmp = new ICmpInst(ICmpInst::ICMP_EQ, objCl, CE, "", ifFalse);
              BasicBlock* notEquals = BasicBlock::Create("", &F);
              BranchInst::Create(ifTrue, notEquals, cmp, ifFalse);
              node->addIncoming(ConstantInt::getTrue(), ifFalse);
              
              if (cl->status < classRead) {
                std::vector<Value*> args;
                args.push_back(objCl);
                args.push_back(CE);
                cmp = CallInst::Create(JnjvmModule::IsAssignableFromFunction,
                                       args.begin(), args.end(), "", notEquals);
                node->addIncoming(cmp, notEquals);
                BranchInst::Create(ifTrue, notEquals);
              } else if (cl->isArray()) {
                std::vector<Value*> args;
                args.push_back(objCl);
                args.push_back(CE);
                Value* res = 
                  CallInst::Create(JnjvmModule::InstantiationOfArrayFunction,
                                   args.begin(), args.end(), "", notEquals);
                node->addIncoming(res, notEquals);
                BranchInst::Create(ifTrue, notEquals);
              } else {
                Value* depthCl;
                if (cl->isResolved()) {
                  depthCl = ConstantInt::get(Type::Int32Ty, cl->depth);
                } else {
                  depthCl = CallInst::Create(JnjvmModule::GetDepthFunction,
                                             CE, "", notEquals);
                }
                Value* depthClObj = CallInst::Create(JnjvmModule::GetDepthFunction,
                                                     objCl, "", notEquals);
                Value* cmp = new ICmpInst(ICmpInst::ICMP_ULE, depthCl, depthClObj, "",
                                          notEquals);
            
                BasicBlock* supDepth = BasicBlock::Create("superior depth", &F);
            
                BranchInst::Create(supDepth, ifTrue, cmp, notEquals);
                node->addIncoming(ConstantInt::getFalse(), notEquals);
  
                Value* inDisplay = 
                  CallInst::Create(JnjvmModule::GetDisplayFunction, objCl,
                                   "", supDepth);
            
                std::vector<Value*> args;
                args.push_back(inDisplay);
                args.push_back(depthCl);
                Value* clInDisplay = 
                  CallInst::Create(JnjvmModule::GetClassInDisplayFunction,
                                   args.begin(), args.end(), "", supDepth);
             
                cmp = new ICmpInst(ICmpInst::ICMP_EQ, clInDisplay, CE, "",
                                   supDepth);
                BranchInst::Create(ifTrue, supDepth); 
                node->addIncoming(cmp, supDepth);
              }
            }
            CI->replaceAllUsesWith(node);
            CI->eraseFromParent();
            BranchInst::Create(NBB, ifTrue);
            break;
          }
        }
#ifdef MULTIPLE_GC
        else if (V == jnjvm::JnjvmModule::GetCollectorFunction) {
          Changed = true;
          Value* val = CI->getOperand(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::jit::constantOne);
          val = new BitCastInst(val, mvm::jit::ptrPtrType, "", CI);
          Value* CollectorPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                          indexes.end(), "",
                                                          CI);
          Value* Collector = new LoadInst(CollectorPtr, "", CI);
          CI->replaceAllUsesWith(Collector);
          CI->eraseFromParent();
        }
#endif
      }
    }
  }
  return Changed;
}


FunctionPass* createLowerConstantCallsPass() {
  return new LowerConstantCalls();
}
  
class VISIBILITY_HIDDEN LowerForcedCalls : public FunctionPass {
  public:
    static char ID;
    LowerForcedCalls() : FunctionPass((intptr_t)&ID) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerForcedCalls::ID = 0;
  static RegisterPass<LowerForcedCalls> Y("LowerForcedCalls",
                                            "Lower Forced calls");

bool LowerForcedCalls::runOnFunction(Function& F) {
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        Value* V = CI->getOperand(0);
        if (V == jnjvm::JnjvmModule::ForceInitialisationCheckFunction) {
          Changed = true;
          CI->eraseFromParent();
        }
      }
    }
  }
  return Changed;
}

FunctionPass* createLowerForcedCallsPass() {
  return new LowerForcedCalls();
}

}
