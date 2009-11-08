//===----- LowerConstantCalls.cpp - Changes arrayLength calls  --------------===//
//
//                               JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

#include "JavaClass.h"
#include "jnjvm/JnjvmModule.h"

using namespace llvm;

namespace jnjvm {

  class VISIBILITY_HIDDEN LowerConstantCalls : public FunctionPass {
  public:
    static char ID;
    JnjvmModule* module;
    LowerConstantCalls(JnjvmModule* M) : FunctionPass((intptr_t)&ID),
      module(M) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerConstantCalls::ID = 0;

#if 0
  static RegisterPass<LowerConstantCalls> X("LowerConstantCalls",
                                            "Lower Constant calls");
#endif


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
  
  Value* IsolateID = GetElementPtrInst::Create(threadId,
      module->OffsetIsolateInThreadConstant, "", CI);

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
  
  Value* IsolateID = GetElementPtrInst::Create(threadId, 
      module->OffsetIsolateInThreadConstant, "", CI);

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
                    module->constantZero };
  Value* TCMArray = GetElementPtrInst::Create(Arg, GEP, GEP + 2, "", CI);
  
  Value* GEP2[2] = { module->constantZero, module->constantZero };

  Value* TCM = GetElementPtrInst::Create(TCMArray, GEP2, GEP2 + 2, "",
                                         CI);
  return new LoadInst(TCM, "", CI);

}
#endif

bool LowerConstantCalls::runOnFunction(Function& F) {
  LLVMContext* Context = &F.getContext();
  bool Changed = false;
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;

      if (ICmpInst* Cmp = dyn_cast<ICmpInst>(I)) {
        if (Cmp->getOperand(1) == module->JavaObjectNullConstant) {
          Value* Arg = Cmp->getOperand(0);
      
#if 0
          // Re-enable this once we can get access of the JavaMethod again.
          if (isVirtual(meth->access) && Arg == F.arg_begin()) {
            Changed = true;
            Cmp->replaceAllUsesWith(ConstantInt::getFalse(*Context));
            Cmp->eraseFromParent();
            break;
          }
#endif
          
          CallSite Ca = CallSite::get(Arg);
          Instruction* CI = Ca.getInstruction();
          if (CI && Ca.getCalledValue() == module->AllocateFunction) {
            Changed = true;
            Cmp->replaceAllUsesWith(ConstantInt::getFalse(*Context));
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
          load = new PtrToIntInst(load, Type::getInt32Ty(*Context), "", CI);
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
                              module->JavaObjectVTOffsetConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, args2, args2 + 2,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          Value* args3[2] = { module->constantZero,
                              module->OffsetClassInVTConstant };

          Value* clPtr = GetElementPtrInst::Create(VT, args3, args3 + 2,
                                                   "", CI);
          Value* cl = new LoadInst(clPtr, "", CI);
          cl = new BitCastInst(cl, module->JavaCommonClassType, "", CI);

          CI->replaceAllUsesWith(cl);
          CI->eraseFromParent();
        } else if (V == module->GetVTFromClassFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0);
          Value* indexes[3] = { module->constantZero, 
                                module->constantZero, 
                                module->OffsetVTInClassConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 3,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetVTFromCommonClassFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0);
          Value* indexes[2] = { module->constantZero, 
                                module->OffsetVTInClassConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetVTFromClassArrayFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0);
          Value* indexes[3] = { module->constantZero,
                                module->constantZero,
                                module->OffsetVTInClassConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 3,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          CI->replaceAllUsesWith(VT);
          CI->eraseFromParent();
        } else if (V == module->GetBaseClassVTFromVTFunction) {
          Changed = true;
          
          Value* val = Call.getArgument(0);
          Value* indexes[2] = { module->constantZero,
                                module->OffsetBaseClassVTInVTConstant };
          Value* VTPtr = GetElementPtrInst::Create(val, indexes, indexes + 2,
                                                   "", CI);
          Value* VT = new LoadInst(VTPtr, "", CI);
          VT = new BitCastInst(VT, module->VTType, "", CI);
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
                                module->OffsetDepthInVTConstant };
          Value* DepthPtr = GetElementPtrInst::Create(val, indexes,
                                                      indexes + 2, "", CI);
          Value* Depth = new LoadInst(DepthPtr, "", CI);
          Depth = new PtrToIntInst(Depth, Type::getInt32Ty(*Context), "", CI);
          CI->replaceAllUsesWith(Depth);
          CI->eraseFromParent();
        } else if (V == module->GetDisplayFunction) {
          Changed = true;
          Value* val = Call.getArgument(0);
          Value* indexes[2] = { module->constantZero,
                                module->OffsetDisplayInVTConstant };
          Value* DisplayPtr = GetElementPtrInst::Create(val, indexes,
                                                        indexes + 2, "", CI);
          const llvm::Type* Ty = PointerType::getUnqual(module->VTType);
          DisplayPtr = new BitCastInst(DisplayPtr, Ty, "", CI);
          CI->replaceAllUsesWith(DisplayPtr);
          CI->eraseFromParent();
        } else if (V == module->GetVTInDisplayFunction) {
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
          Value* cmp = new ICmpInst(CI, ICmpInst::ICMP_EQ, Del, 
                                    module->JavaObjectNullConstant, "");
          
          BasicBlock* NoDelegatee = BasicBlock::Create(*Context, "No delegatee", &F);
          BasicBlock* DelegateeOK = BasicBlock::Create(*Context, "Delegatee OK", &F);
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
          Value* GEP[2] = 
            { module->constantZero,
              module->OffsetInitializedInTaskClassMirrorConstant };
          Value* StatusPtr = GetElementPtrInst::Create(TCM, GEP, GEP + 2, "",
                                                       CI);
          
          Value* test = new LoadInst(StatusPtr, "", CI);
          
          BasicBlock* trueCl = BasicBlock::Create(*Context, "Initialized", &F);
          BasicBlock* falseCl = BasicBlock::Create(*Context, "Uninitialized", &F);
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
          Value* indexes = ConstantInt::get(Type::getInt32Ty(*Context), val + 1);
#else
          Value* indexes = Index;
#endif
          Value* arg1 = GetElementPtrInst::Create(CTP, indexes, "", CI);
          arg1 = new LoadInst(arg1, "", false, CI);
          Value* test = new ICmpInst(CI, ICmpInst::ICMP_EQ, arg1,
                                     module->constantPtrNull, "");
 
          BasicBlock* trueCl = BasicBlock::Create(*Context, "Ctp OK", &F);
          BasicBlock* falseCl = BasicBlock::Create(*Context, "Ctp Not OK", &F);
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

            Constant* init = Constant::getNullValue(module->JavaClassArrayType);
            GlobalVariable* GV = 
              new GlobalVariable(*(F.getParent()), module->JavaClassArrayType,
                                 false, GlobalValue::ExternalLinkage,
                                 init, "");

            Value* LoadedGV = new LoadInst(GV, "", CI);
            Value* cmp = new ICmpInst(CI, ICmpInst::ICMP_EQ, LoadedGV, init,
                                      "");

            BasicBlock* OKBlock = BasicBlock::Create(*Context, "", &F);
            BasicBlock* NotOKBlock = BasicBlock::Create(*Context, "", &F);
            PHINode* node = PHINode::Create(module->JavaClassArrayType, "",
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
        } else if (V == module->ForceInitialisationCheckFunction ||
                   V == module->ForceLoadedCheckFunction ) {
          Changed = true;
          CI->eraseFromParent();
        } else if (V == module->GetFinalInt8FieldFunction ||
                   V == module->GetFinalInt16FieldFunction ||
                   V == module->GetFinalInt32FieldFunction ||
                   V == module->GetFinalLongFieldFunction ||
                   V == module->GetFinalFloatFieldFunction ||
                   V == module->GetFinalDoubleFieldFunction ||
                   V == module->GetFinalObjectFieldFunction) {
          Changed = true;
          Value* val = Call.getArgument(0);
          Value* res = new LoadInst(val, "", CI);
          CI->replaceAllUsesWith(res);
          CI->eraseFromParent();
        } else if (V == module->IsAssignableFromFunction) {
          Changed = true;
          Value* VT1 = Call.getArgument(0);
          Value* VT2 = Call.getArgument(1);
          
          BasicBlock* EndBlock = II->getParent()->splitBasicBlock(II);
          I->getParent()->getTerminator()->eraseFromParent();
          
          BasicBlock* CurEndBlock = BasicBlock::Create(*Context, "", &F);
          BasicBlock* FailedBlock = BasicBlock::Create(*Context, "", &F);
          PHINode* node = PHINode::Create(Type::getInt1Ty(*Context), "", CurEndBlock);

          ConstantInt* CC = ConstantInt::get(Type::getInt32Ty(*Context),
              JavaVirtualTable::getOffsetIndex());
          Value* indices[2] = { module->constantZero, CC };
          Value* Offset = GetElementPtrInst::Create(VT2, indices, indices + 2,
                                                    "", CI);
          Offset = new LoadInst(Offset, "", false, CI);
          Offset = new PtrToIntInst(Offset, Type::getInt32Ty(*Context), "", CI);
          indices[1] = Offset;
          Value* CurVT = GetElementPtrInst::Create(VT1, indices, indices + 2,
                                                   "", CI);
          CurVT = new LoadInst(CurVT, "", false, CI);
          CurVT = new BitCastInst(CurVT, module->VTType, "", CI);
             
          Value* res = new ICmpInst(CI, ICmpInst::ICMP_EQ, CurVT, VT2, "");

          node->addIncoming(ConstantInt::getTrue(*Context), CI->getParent());
          BranchInst::Create(CurEndBlock, FailedBlock, res, CI);

          Value* Args[2] = { VT1, VT2 };
          res = CallInst::Create(module->IsSecondaryClassFunction, Args,
                                 Args + 2, "", FailedBlock);
         
          node->addIncoming(res, FailedBlock);
          BranchInst::Create(CurEndBlock, FailedBlock);

          // Branch to the next block.
          BranchInst::Create(EndBlock, CurEndBlock);
          
          // We can now replace the previous instruction.
          CI->replaceAllUsesWith(node);
          CI->eraseFromParent();
          
          // Reanalyse the current block.
          break;

        } else if (V == module->IsSecondaryClassFunction) {
          Changed = true;
          Value* VT1 = Call.getArgument(0);
          Value* VT2 = Call.getArgument(1);
            
          BasicBlock* EndBlock = II->getParent()->splitBasicBlock(II);
          I->getParent()->getTerminator()->eraseFromParent();


          BasicBlock* Preheader = BasicBlock::Create(*Context, "preheader", &F);
          BasicBlock* BB4 = BasicBlock::Create(*Context, "BB4", &F);
          BasicBlock* BB5 = BasicBlock::Create(*Context, "BB5", &F);
          BasicBlock* BB6 = BasicBlock::Create(*Context, "BB6", &F);
          BasicBlock* BB7 = BasicBlock::Create(*Context, "BB7", &F);
          BasicBlock* BB9 = BasicBlock::Create(*Context, "BB9", &F);
          const Type* Ty = PointerType::getUnqual(module->VTType);
          
          PHINode* resFwd = PHINode::Create(Type::getInt32Ty(*Context), "", BB7);
   
          // This corresponds to:
          //    if (VT1.cache == VT2 || VT1 == VT2) goto end with true;
          //    else goto headerLoop;
          ConstantInt* cacheIndex = 
            ConstantInt::get(Type::getInt32Ty(*Context), JavaVirtualTable::getCacheIndex());
          Value* indices[2] = { module->constantZero, cacheIndex };
          Instruction* CachePtr = 
            GetElementPtrInst::Create(VT1, indices, indices + 2, "", CI);
          CachePtr = new BitCastInst(CachePtr, Ty, "", CI);
          Value* Cache = new LoadInst(CachePtr, "", false, CI);
          ICmpInst* cmp1 = new ICmpInst(CI, ICmpInst::ICMP_EQ, Cache, VT2, "");
          ICmpInst* cmp2 = new ICmpInst(CI, ICmpInst::ICMP_EQ, VT1, VT2, "");
          BinaryOperator* Or = BinaryOperator::Create(Instruction::Or, cmp1,
                                                      cmp2, "", CI);
          BranchInst::Create(BB9, Preheader, Or, CI);
    
          // First test failed. Go into the loop. The Preheader looks like this:
          // headerLoop:
          //    types = VT1->secondaryTypes;
          //    size = VT1->nbSecondaryTypes;
          //    i = 0;
          //    goto test;
          ConstantInt* sizeIndex = ConstantInt::get(Type::getInt32Ty(*Context), 
              JavaVirtualTable::getNumSecondaryTypesIndex());
          indices[1] = sizeIndex;
          Instruction* Size = GetElementPtrInst::Create(VT1, indices,
                                                        indices + 2, "",
                                                        Preheader);
          Size = new LoadInst(Size, "", false, Preheader);
          Size = new PtrToIntInst(Size, Type::getInt32Ty(*Context), "", Preheader);
    
          ConstantInt* secondaryTypesIndex = ConstantInt::get(Type::getInt32Ty(*Context), 
              JavaVirtualTable::getSecondaryTypesIndex());
          indices[1] = secondaryTypesIndex;
          Instruction* secondaryTypes = 
            GetElementPtrInst::Create(VT1, indices, indices + 2, "", Preheader);
          secondaryTypes = new LoadInst(secondaryTypes, "", false, Preheader);
          secondaryTypes = new BitCastInst(secondaryTypes, Ty, "", Preheader);
          BranchInst::Create(BB7, Preheader);
    
          // Here is the test if the current secondary type is VT2.
          // test:
          //   CurVT = types[i];
          //   if (CurVT == VT2) goto update cache;
          //   est goto inc;
          Instruction* CurVT = GetElementPtrInst::Create(secondaryTypes, resFwd,
                                                         "", BB4);
          CurVT = new LoadInst(CurVT, "", false, BB4);
          cmp1 = new ICmpInst(*BB4, ICmpInst::ICMP_EQ, CurVT, VT2, "");
          BranchInst::Create(BB5, BB6, cmp1, BB4);
    
          // Increment i if the previous test failed
          // inc:
          //    ++i;
          //    goto endLoopTest;
          BinaryOperator* IndVar = 
            BinaryOperator::CreateAdd(resFwd, module->constantOne, "", BB6);
          BranchInst::Create(BB7, BB6);
    
          // Verify that we haven't reached the end of the loop:
          // endLoopTest:
          //    if (i < size) goto test
          //    else goto end with false
          resFwd->reserveOperandSpace(2);
          resFwd->addIncoming(module->constantZero, Preheader);
          resFwd->addIncoming(IndVar, BB6);
    
          cmp1 = new ICmpInst(*BB7, ICmpInst::ICMP_SGT, Size, resFwd, "");
          BranchInst::Create(BB4, BB9, cmp1, BB7);
   
          // Update the cache if the result is found.
          // updateCache:
          //    VT1->cache = result
          //    goto end with true
          new StoreInst(VT2, CachePtr, false, BB5);
          BranchInst::Create(BB9, BB5);

          // Final block, that gets the result.
          PHINode* node = PHINode::Create(Type::getInt1Ty(*Context), "", BB9);
          node->reserveOperandSpace(3);
          node->addIncoming(ConstantInt::getTrue(*Context), CI->getParent());
          node->addIncoming(ConstantInt::getFalse(*Context), BB7);
          node->addIncoming(ConstantInt::getTrue(*Context), BB5);
    
          // Don't forget to jump to the next block.
          BranchInst::Create(EndBlock, BB9);
   
          // We can now replace the previous instruction
          CI->replaceAllUsesWith(node);
          CI->eraseFromParent();

          // And reanalyse the current block.
          break;
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


FunctionPass* createLowerConstantCallsPass(JnjvmModule* M) {
  return new LowerConstantCalls(M);
}

}
