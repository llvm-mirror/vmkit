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

#include "JnjvmModule.h"

using namespace llvm;

namespace jnjvm {

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


#ifdef ISOLATE
static Value* getTCM(JnjvmModule* module, Value* Arg, Instruction* CI) {
  Value* GEP[2] = { module->constantZero,
                    module->OffsetTaskClassMirrorInClassConstant };
  Value* TCMArray = GetElementPtrInst::Create(Arg, GEP, GEP + 2, "", CI);

  Value* threadId = CallInst::Create(module->llvm_frameaddress,
                                     module->constantZero, "", CI);
  threadId = new PtrToIntInst(threadId, module->pointerSizeType, "", CI);
  threadId = BinaryOperator::CreateAnd(threadId, module->constantThreadIDMask,
                                       "", CI);
  
  threadId = new IntToPtrInst(threadId, module->ptr32Type, "", CI);
  
  Value* IsolateID = GetElementPtrInst::Create(threadId, module->constantThree,
                                               "", CI);
  IsolateID = new LoadInst(IsolateID, "", CI);

  Value* GEP2[2] = { module->constantZero, IsolateID };

  Value* TCM = GetElementPtrInst::Create(TCMArray, GEP2, GEP2 + 2, "",
                                         CI);
  return TCM;
}

static Value* getDelegatee(JnjvmModule* module, Value* Arg, Instruction* CI) {
  Value* GEP[2] = { module->constantZero,
                    module->constantTwo };
  Value* TCMArray = GetElementPtrInst::Create(Arg, GEP, GEP + 2, "", CI);

  Value* threadId = CallInst::Create(module->llvm_frameaddress,
                                     module->constantZero, "", CI);
  threadId = new PtrToIntInst(threadId, module->pointerSizeType, "", CI);
  threadId = BinaryOperator::CreateAnd(threadId, module->constantThreadIDMask,
                                       "", CI);
  
  threadId = new IntToPtrInst(threadId, module->ptr32Type, "", CI);
  
  Value* IsolateID = GetElementPtrInst::Create(threadId, module->constantThree,
                                               "", CI);
  IsolateID = new LoadInst(IsolateID, "", CI);

  Value* GEP2[2] = { module->constantZero, IsolateID };

  Value* TCM = GetElementPtrInst::Create(TCMArray, GEP2, GEP2 + 2, "",
                                         CI);
  return new LoadInst(TCM, "", CI);
}

#else

static Value* getTCM(JnjvmModule* module, Value* Arg, Instruction* CI) {
  Value* GEP[2] = { module->constantZero,
                    module->OffsetTaskClassMirrorInClassConstant };
  Value* TCMArray = GetElementPtrInst::Create(Arg, GEP, GEP + 2, "", CI);
  
  Value* GEP2[2] = { module->constantZero, module->constantZero };

  Value* TCM = GetElementPtrInst::Create(TCMArray, GEP2, GEP2 + 2, "",
                                         CI);
  return TCM;

}

static Value* getDelegatee(JnjvmModule* module, Value* Arg, Instruction* CI) {
  Value* GEP[2] = { module->constantZero,
                    module->constantTwo };
  Value* TCMArray = GetElementPtrInst::Create(Arg, GEP, GEP + 2, "", CI);
  
  Value* GEP2[2] = { module->constantZero, module->constantZero };

  Value* TCM = GetElementPtrInst::Create(TCMArray, GEP2, GEP2 + 2, "",
                                         CI);
  return new LoadInst(TCM, "", CI);

}
#endif

bool LowerConstantCalls::runOnFunction(Function& F) {
  JnjvmModule* module = (JnjvmModule*)F.getParent();
  JavaMethod* meth = LLVMMethodInfo::get(&F);
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;

      if (ICmpInst* Cmp = dyn_cast<ICmpInst>(I)) {
        if (isVirtual(meth->access)) {
          if (Cmp->getOperand(1) == module->JavaObjectNullConstant && 
              Cmp->getOperand(0) == F.arg_begin()) {
            Changed = true;
            Cmp->replaceAllUsesWith(ConstantInt::getFalse());
            Cmp->eraseFromParent();
            break;
          }
        }
      }

      CallSite Call = CallSite::get(I);
      Instruction* CI = Call.getInstruction();
      if (CI) {
        Value* V = Call.getCalledValue();
        if (V == module->ArrayLengthFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the array
          Value* array = new BitCastInst(val, module->JavaArrayType,
                                         "", CI);
          Value* args[2] = { module->constantZero, 
                             module->JavaArraySizeOffsetConstant };
          Value* ptr = GetElementPtrInst::Create(array, args, args + 2,
                                                 "", CI);
          Value* load = new LoadInst(ptr, "", CI);
          load = new PtrToIntInst(load, Type::Int32Ty, "", CI);
          CI->replaceAllUsesWith(load);
          CI->eraseFromParent();
        } else if (V == module->GetVTFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the object
          Value* indexes[2] = { module->constantZero, module->constantZero };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); // get the object
          Value* args2[2] = { module->constantZero,
                              module->JavaObjectClassOffsetConstant };
          Value* classPtr = GetElementPtrInst::Create(val, args2, args2 + 2,
                                                      "", CI);
          Value* cl = new LoadInst(classPtr, "", CI);
          CI->replaceAllUsesWith(cl);
          CI->eraseFromParent();
        } else if (V == module->GetVTFromClassFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0);
          Value* indexes[2] = { module->constantZero, 
                                module->OffsetVTInClassConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetObjectSizeFromClassFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0); 
          Value* indexes[2] = { module->constantZero, 
                                module->OffsetObjectSizeInClassConstant };
          Value* SizePtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                     "", CI);
          Value* Size = new LoadInst(SizePtr, "", CI);
          CI->replaceAllUsesWith(Size);
          CI->eraseFromParent();
        } else if (V == module->GetDepthFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* indexes[2] = { module->constantZero,
                                module->OffsetDepthInClassConstant };
          Value* DepthPtr = GetElementPtrInst::Create(val, indexes,
                                                      indexes + 2, "", CI);
          Value* Depth = new LoadInst(DepthPtr, "", CI);
          CI->replaceAllUsesWith(Depth);
          CI->eraseFromParent();
        } else if (V == module->GetDisplayFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* indexes[2] = { module->constantZero,
                                module->OffsetDisplayInClassConstant };
          Value* DisplayPtr = GetElementPtrInst::Create(val, indexes,
                                                        indexes + 2, "", CI);
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
#if defined(ISOLATE)
        } else if (V == module->GetStaticInstanceFunction) {
          Changed = true;

          Value* TCM = getTCM(module, Call.getArgument(0), CI);
          Constant* C = module->OffsetStaticInstanceInTaskClassMirrorConstant;
          Value* GEP[2] = { module->constantZero, C };
          Value* Replace = GetElementPtrInst::Create(TCM, GEP, GEP + 2, "", CI);
          Replace = new LoadInst(Replace, "", CI);
          CI->replaceAllUsesWith(Replace);
          CI->eraseFromParent();
#endif
        } else if (V == module->GetClassDelegateeFunction) {
          Changed = true;
          BasicBlock* NBB = II->getParent()->splitBasicBlock(II);
          I->getParent()->getTerminator()->eraseFromParent();
          Value* Del = getDelegatee(module, Call.getArgument(0), CI);
          Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, Del, 
                                    module->JavaObjectNullConstant, "", CI);
          
          BasicBlock* NoDelegatee = BasicBlock::Create("No delegatee", &F);
          BasicBlock* DelegateeOK = BasicBlock::Create("Delegatee OK", &F);
          BranchInst::Create(NoDelegatee, DelegateeOK, cmp, CI);
          PHINode* phi = PHINode::Create(module->JavaObjectType, "", DelegateeOK);
          phi->addIncoming(Del, CI->getParent());
          
          Value* Res = CallInst::Create(module->RuntimeDelegateeFunction,
                                        Call.getArgument(0), "", NoDelegatee);
          BranchInst::Create(DelegateeOK, NoDelegatee);
          phi->addIncoming(Res, NoDelegatee);

          CI->replaceAllUsesWith(phi);
          CI->eraseFromParent();
          BranchInst::Create(NBB, DelegateeOK);
          break;
         
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
          Value* TCM = getTCM(module, Call.getArgument(0), CI);
          Value* GEP[2] = { module->constantZero,
                            module->OffsetStatusInTaskClassMirrorConstant };
          Value* StatusPtr = GetElementPtrInst::Create(TCM, GEP, GEP + 2, "",
                                                       CI);
          
          Value* Status = new LoadInst(StatusPtr, "", CI);
          
          Value* test = new ICmpInst(ICmpInst::ICMP_EQ, Status,
                                     jnjvm::JnjvmModule::ClassReadyConstant,
                                     "", CI);
 
          BasicBlock* trueCl = BasicBlock::Create("Initialized", &F);
          BasicBlock* falseCl = BasicBlock::Create("Uninitialized", &F);
          PHINode* node = llvm::PHINode::Create(JnjvmModule::JavaClassType, "", trueCl);
          node->addIncoming(Cl, CI->getParent());
          BranchInst::Create(trueCl, falseCl, test, CI);
  
          
          Value* res = 0;
          if (InvokeInst* Invoke = dyn_cast<InvokeInst>(CI)) {
            Value* Args[1] = { Cl };
            BasicBlock* UI = Invoke->getUnwindDest();
            res = InvokeInst::Create(module->InitialiseClassFunction,
                                     trueCl, UI, Args, Args + 1,
                                     "", falseCl);

            // For some reason, an LLVM pass may add PHI nodes to the
            // exception destination.
            BasicBlock::iterator Temp = UI->getInstList().begin();
            while (PHINode* PHI = dyn_cast<PHINode>(Temp)) {
              Value* Val = PHI->getIncomingValueForBlock(CI->getParent());
              PHI->removeIncomingValue(CI->getParent(), false);
              PHI->addIncoming(Val, falseCl);
              Temp++;
            }
            
            // And here we set the phi nodes of the normal dest of the Invoke
            // instruction. The phi nodes have now the trueCl as basic block.
            Temp = NBB->getInstList().begin();
            while (PHINode* PHI = dyn_cast<PHINode>(Temp)) {
              Value* Val = PHI->getIncomingValueForBlock(CI->getParent());
              PHI->removeIncomingValue(CI->getParent(), false);
              PHI->addIncoming(Val, trueCl);
              Temp++;
            }
          } else {
            res = CallInst::Create(module->InitialiseClassFunction,
                                   Cl, "", falseCl);
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
          
#ifdef ISOLATE_SHARING
          ConstantInt* Cons = dyn_cast<ConstantInt>(Index);
          assert(CI && "Wrong use of GetConstantPoolAt");
          uint64 val = Cons->getZExtValue();
          Value* indexes = ConstantInt::get(Type::Int32Ty, val + 1);
#else
          Value* indexes = Index;
#endif
          Value* arg1 = GetElementPtrInst::Create(CTP, indexes, "", CI);
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

            // And here we set the phi nodes of the normal dest of the Invoke
            // instruction. The phi nodes have now the trueCl as basic block.
            Temp = NBB->getInstList().begin();
            while (PHINode* PHI = dyn_cast<PHINode>(Temp)) {
              Value* Val = PHI->getIncomingValueForBlock(CI->getParent());
              PHI->removeIncomingValue(CI->getParent(), false);
              PHI->addIncoming(Val, trueCl);
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
        } else if (V == module->GetArrayClassFunction) {
          const llvm::Type* Ty = 
            PointerType::getUnqual(module->JavaCommonClassType);
          Constant* nullValue = Constant::getNullValue(Ty);
          // Check if we have already proceed this call.
          if (Call.getArgument(1) == nullValue) { 
            BasicBlock* NBB = II->getParent()->splitBasicBlock(II);
            I->getParent()->getTerminator()->eraseFromParent();

            Constant* init = 
              Constant::getNullValue(module->JavaCommonClassType);
            GlobalVariable* GV = 
              new GlobalVariable(module->JavaCommonClassType, false,
                                 GlobalValue::ExternalLinkage,
                                 init, "", module);

            Value* LoadedGV = new LoadInst(GV, "", CI);
            Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, LoadedGV, init,
                                      "", CI);

            BasicBlock* OKBlock = BasicBlock::Create("", &F);
            BasicBlock* NotOKBlock = BasicBlock::Create("", &F);
            PHINode* node = PHINode::Create(module->JavaCommonClassType, "",
                                            OKBlock);
            node->addIncoming(LoadedGV, CI->getParent());

            BranchInst::Create(NotOKBlock, OKBlock, cmp, CI);

            Value* args[2] = { Call.getArgument(0), GV };
            Value* res = CallInst::Create(module->GetArrayClassFunction, args,
                                          args + 2, "", NotOKBlock);
            BranchInst::Create(OKBlock, NotOKBlock);
            node->addIncoming(res, NotOKBlock);
            
            CI->replaceAllUsesWith(node);
            CI->eraseFromParent();
            BranchInst::Create(NBB, OKBlock);
            break;
          }
        } else if (V == module->ForceInitialisationCheckFunction) {
          Changed = true;
          CI->eraseFromParent();
        }
#ifdef ISOLATE_SHARING
        else if (V == module->GetCtpClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* indexes[2] = { module->constantZero, 
                                module->OffsetCtpInClassConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes,
                                                   indexes + 2, "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetCtpCacheNodeFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* indexes[2] = { module->constantZero, module->constantFour };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes,
                                                   indexes + 2, "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetJnjvmArrayClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0); 
          Value* index = Call.getArgument(1); 
          Value* indexes[3] = { module->constantZero, module->constantTwo,
                                index };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 3,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetJnjvmExceptionClassFunction) {
          Changed = true;
          Value* val = Call.getArgument(0);
          Value* indexes[2] = { module->constantZero, module->constantOne };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                   "", CI);
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

}
