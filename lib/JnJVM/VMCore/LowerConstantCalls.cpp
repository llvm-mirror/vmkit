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
#include "llvm/Support/CallSite.h"
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
  JnjvmModule* module = (JnjvmModule*)F.getParent();
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      CallSite Call = CallSite::get(I);
      Instruction* CI = Call.getInstruction();
      if (CI) {
        Value* V = Call.getCalledValue();
        if (V == module->ArrayLengthFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the array
          Value* array = new BitCastInst(val, module->JavaArrayType,
                                         "", CI);
          std::vector<Value*> args; //size=  2
          args.push_back(mvm::MvmModule::constantZero);
          args.push_back(module->JavaArraySizeOffsetConstant);
          Value* ptr = GetElementPtrInst::Create(array, args.begin(), args.end(),
                                         "", CI);
          Value* load = new LoadInst(ptr, "", CI);
          CI->replaceAllUsesWith(load);
          CI->eraseFromParent();
        } else if (V == module->GetVTFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the object
          std::vector<Value*> indexes; //[3];
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(mvm::MvmModule::constantZero);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                           indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the object
          std::vector<Value*> args2;
          args2.push_back(mvm::MvmModule::constantZero);
          args2.push_back(module->JavaObjectClassOffsetConstant);
          Value* classPtr = GetElementPtrInst::Create(val, args2.begin(),
                                                      args2.end(), "",
                                                      CI);
          Value* cl = new LoadInst(classPtr, "", CI);
          CI->replaceAllUsesWith(cl);
          CI->eraseFromParent();
        } else if (V == module->GetVTFromClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetVTInClassConstant);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetObjectSizeFromClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetObjectSizeInClassConstant);
          Value* SizePtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* Size = new LoadInst(SizePtr, "", CI);
          CI->replaceAllUsesWith(Size);
          CI->eraseFromParent();
        } else if (V == module->GetDepthFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetDepthInClassConstant);
          Value* DepthPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                      indexes.end(), "", CI);
          Value* Depth = new LoadInst(DepthPtr, "", CI);
          CI->replaceAllUsesWith(Depth);
          CI->eraseFromParent();
        } else if (V == module->GetDisplayFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetDisplayInClassConstant);
          Value* DisplayPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                        indexes.end(), "", CI);
          Value* Display = new LoadInst(DisplayPtr, "", CI);
          CI->replaceAllUsesWith(Display);
          CI->eraseFromParent();
        } else if (V == module->GetClassInDisplayFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* depth = Call.getArgument(1); 
          Value* ClassPtr = GetElementPtrInst::Create(val, depth, "", CI);
          Value* Class = new LoadInst(ClassPtr, "", CI);
          CI->replaceAllUsesWith(Class);
          CI->eraseFromParent();
        } else if (V == module->InstanceOfFunction) {
          ConstantExpr* CE = dyn_cast<ConstantExpr>(Call.getArgument(1));
          if (CE) {
            ConstantInt* C = (ConstantInt*)CE->getOperand(0);
            CommonClass* cl = (CommonClass*)C->getZExtValue();
            Changed = true;
            BasicBlock* NBB = II->getParent()->splitBasicBlock(II);
            I->getParent()->getTerminator()->eraseFromParent();
            Value* obj = Call.getArgument(0);
            Instruction* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj,
                                            module->JavaObjectNullConstant,
                                            "", CI);
            BasicBlock* ifTrue = BasicBlock::Create("", &F);
            BasicBlock* ifFalse = BasicBlock::Create("", &F);
            BranchInst::Create(ifTrue, ifFalse, cmp, CI);
            PHINode* node = PHINode::Create(Type::Int1Ty, "", ifTrue);
            node->addIncoming(ConstantInt::getFalse(), CI->getParent());
            Value* objCl = CallInst::Create(module->GetClassFunction, obj,
                                            "", ifFalse);
            
            if (isInterface(cl->access)) {
              std::vector<Value*> args;
              args.push_back(objCl);
              args.push_back(CE);
              Value* res = CallInst::Create(module->ImplementsFunction,
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
                cmp = CallInst::Create(module->IsAssignableFromFunction,
                                       args.begin(), args.end(), "", notEquals);
                node->addIncoming(cmp, notEquals);
                BranchInst::Create(ifTrue, notEquals);
              } else if (cl->isArray()) {
                std::vector<Value*> args;
                args.push_back(objCl);
                args.push_back(CE);
                Value* res = 
                  CallInst::Create(module->InstantiationOfArrayFunction,
                                   args.begin(), args.end(), "", notEquals);
                node->addIncoming(res, notEquals);
                BranchInst::Create(ifTrue, notEquals);
              } else {
                Value* depthCl;
                if (cl->isResolved()) {
                  depthCl = ConstantInt::get(Type::Int32Ty, cl->depth);
                } else {
                  depthCl = CallInst::Create(module->GetDepthFunction,
                                             CE, "", notEquals);
                }
                Value* depthClObj = CallInst::Create(module->GetDepthFunction,
                                                     objCl, "", notEquals);
                Value* cmp = new ICmpInst(ICmpInst::ICMP_ULE, depthCl, depthClObj, "",
                                          notEquals);
            
                BasicBlock* supDepth = BasicBlock::Create("superior depth", &F);
            
                BranchInst::Create(supDepth, ifTrue, cmp, notEquals);
                node->addIncoming(ConstantInt::getFalse(), notEquals);
  
                Value* inDisplay = 
                  CallInst::Create(module->GetDisplayFunction, objCl,
                                   "", supDepth);
            
                std::vector<Value*> args;
                args.push_back(inDisplay);
                args.push_back(depthCl);
                Value* clInDisplay = 
                  CallInst::Create(module->GetClassInDisplayFunction,
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
        } else if (V == module->InitialisationCheckFunction) {
          Changed = true;
          
          BasicBlock* NBB = 0;
          if (CI->getParent()->getTerminator() != CI) {
            NBB = II->getParent()->splitBasicBlock(II);
            CI->getParent()->getTerminator()->eraseFromParent();
          } else {
            InvokeInst* Invoke = dyn_cast<InvokeInst>(CI);
            assert(Invoke && "Last instruction is not an invoke");
            NBB = Invoke->getNormalDest();
          }
          
          Value* Cl = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetStatusInClassConstant);
          Value* StatusPtr = GetElementPtrInst::Create(Cl, indexes.begin(),
                                                       indexes.end(), "", CI);
          Value* Status = new LoadInst(StatusPtr, "", CI);
          
          
          Value* test = new ICmpInst(ICmpInst::ICMP_UGT, Status,
                                     jnjvm::JnjvmModule::ClassReadyConstant,
                                     "", CI);
 
          BasicBlock* trueCl = BasicBlock::Create("Initialized", &F);
          BasicBlock* falseCl = BasicBlock::Create("Uninitialized", &F);
          PHINode* node = llvm::PHINode::Create(JnjvmModule::JavaClassType, "", trueCl);
          node->addIncoming(Cl, CI->getParent());
          BranchInst::Create(trueCl, falseCl, test, CI);
  
          std::vector<Value*> Args;
          Args.push_back(Cl);
          
          Value* res = 0;
          if (InvokeInst* Invoke = dyn_cast<InvokeInst>(CI)) {
            BasicBlock* UI = Invoke->getUnwindDest();
            res = InvokeInst::Create(module->InitialiseClassFunction,
                                     trueCl, UI, Args.begin(),
                                     Args.end(), "", falseCl);

            // For some reason, an LLVM pass may add PHI nodes to the
            // exception destination.
            BasicBlock::iterator Temp = UI->getInstList().begin();
            while (PHINode* PHI = dyn_cast<PHINode>(Temp)) {
              Value* Val = PHI->getIncomingValueForBlock(CI->getParent());
              PHI->removeIncomingValue(CI->getParent(), false);
              PHI->addIncoming(Val, falseCl);
              Temp++;
            }
          } else {
            res = CallInst::Create(module->InitialiseClassFunction,
                                   Args.begin(), Args.end(), "",
                                   falseCl);
            BranchInst::Create(trueCl, falseCl);
          }
          
          node->addIncoming(res, falseCl);


          CI->replaceAllUsesWith(node);
          CI->eraseFromParent();
          BranchInst::Create(NBB, trueCl);
          break;
        } else if (V == module->GetConstantPoolAtFunction) {
          Function* resolver = dyn_cast<Function>(Call.getArgument(0));
          assert(resolver && "Wrong use of GetConstantPoolAt");
          const Type* returnType = resolver->getReturnType();
          Value* CTP = Call.getArgument(1);
          Value* Index = Call.getArgument(3);
          Changed = true;
          BasicBlock* NBB = 0;
          if (CI->getParent()->getTerminator() != CI) {
            NBB = II->getParent()->splitBasicBlock(II);
            CI->getParent()->getTerminator()->eraseFromParent();
          } else {
            InvokeInst* Invoke = dyn_cast<InvokeInst>(CI);
            assert(Invoke && "Last instruction is not an invoke");
            NBB = Invoke->getNormalDest();
          }
          
          std::vector<Value*> indexes; //[3];
#ifdef ISOLATE_SHARING
          ConstantInt* Cons = dyn_cast<ConstantInt>(Index);
          assert(CI && "Wrong use of GetConstantPoolAt");
          uint64 val = Cons->getZExtValue();
          indexes.push_back(ConstantInt::get(Type::Int32Ty, val + 1));
#else
          indexes.push_back(Index);
#endif
          Value* arg1 = GetElementPtrInst::Create(CTP, indexes.begin(),
                                                  indexes.end(),  "", CI);
          arg1 = new LoadInst(arg1, "", false, CI);
          Value* test = new ICmpInst(ICmpInst::ICMP_EQ, arg1,
                                     mvm::MvmModule::constantPtrNull, "", CI);
 
          BasicBlock* trueCl = BasicBlock::Create("Ctp OK", &F);
          BasicBlock* falseCl = BasicBlock::Create("Ctp Not OK", &F);
          PHINode* node = llvm::PHINode::Create(returnType, "", trueCl);
          node->addIncoming(arg1, CI->getParent());
          BranchInst::Create(falseCl, trueCl, test, CI);
  
          std::vector<Value*> Args;
          unsigned ArgSize = Call.arg_size(), i = 1;
          while (++i < ArgSize) {
            Args.push_back(Call.getArgument(i));
          }
          
          Value* res = 0;
          if (InvokeInst* Invoke = dyn_cast<InvokeInst>(CI)) {
            BasicBlock* UI = Invoke->getUnwindDest();
            res = InvokeInst::Create(resolver, trueCl, UI, Args.begin(),
                                     Args.end(), "", falseCl);

            // For some reason, an LLVM pass may add PHI nodes to the
            // exception destination.
            BasicBlock::iterator Temp = UI->getInstList().begin();
            while (PHINode* PHI = dyn_cast<PHINode>(Temp)) {
              Value* Val = PHI->getIncomingValueForBlock(CI->getParent());
              PHI->removeIncomingValue(CI->getParent(), false);
              PHI->addIncoming(Val, falseCl);
              Temp++;
            }
          } else {
            res = CallInst::Create(resolver, Args.begin(), Args.end(), "",
                                   falseCl);
            BranchInst::Create(trueCl, falseCl);
          }
          
          node->addIncoming(res, falseCl);


          CI->replaceAllUsesWith(node);
          CI->eraseFromParent();
          BranchInst::Create(NBB, trueCl);
          break;
        }
#ifdef MULTIPLE_GC
        else if (V == module->GetCollectorFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantOne);
          val = new BitCastInst(val, mvm::MvmModule::ptrPtrType, "", CI);
          Value* CollectorPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                          indexes.end(), "",
                                                          CI);
          Value* Collector = new LoadInst(CollectorPtr, "", CI);
          CI->replaceAllUsesWith(Collector);
          CI->eraseFromParent();
        }
#endif

#ifdef ISOLATE_SHARING
        else if (V == module->GetCtpClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(module->OffsetCtpInClassConstant);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetCtpCacheNodeFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(mvm::MvmModule::constantFour);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetJnjvmArrayClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* index = Call.getArgument(1); 
          std::vector<Value*> indexes; 
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(mvm::MvmModule::constantTwo);
          indexes.push_back(index);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetJnjvmExceptionClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0);
          std::vector<Value*> indexes;
          indexes.push_back(mvm::MvmModule::constantZero);
          indexes.push_back(mvm::MvmModule::constantOne);
          Value* VTPtr = GetElementPtrInst::Create(val, indexes.begin(),
                                                   indexes.end(), "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
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
  JnjvmModule* module = (JnjvmModule*)F.getParent();
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        Value* V = CI->getOperand(0);
        if (V == module->ForceInitialisationCheckFunction) {
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
