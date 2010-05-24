//===-- LowerJavaRT.cpp - Remove references to RT classes and functions  --===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

  class LowerJavaRT : public ModulePass {
  public:
    static char ID;
    LowerJavaRT() : ModulePass((intptr_t)&ID) { }

    virtual bool runOnModule(Module &M);
  private:
  };
  char LowerJavaRT::ID = 0;
  static RegisterPass<LowerJavaRT> X("LowerJavaRT",
                                     "Remove references to RT");

bool LowerJavaRT::runOnModule(Module& M) {
  bool Changed = true;

  for (Module::iterator I = M.begin(), E = M.end(); I != E;) {
    Function& GV = *I;
    ++I;
    if (!strncmp(GV.getName().data(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().data(), "java", 4)) {
      if (!strcmp(GV.getName().data(), "JnJVM_java_lang_String_charAt__I")) {
  	    Function* F = M.getFunction("MMTkCharAt");
        if (!F) 
          F = Function::Create(GV.getFunctionType(),
                               GlobalValue::ExternalLinkage, "MMTkCharAt", &M);
      	
        GV.replaceAllUsesWith(F);
      } else if (!strcmp(GV.getName().data(), "JnJVM_java_lang_Object_getClass__")) {
  	    Function* F = M.getFunction("MMTkGetClass");
	      if (!F) 
          F = Function::Create(GV.getFunctionType(),
                               GlobalValue::ExternalLinkage, "MMTkGetClass", &M);
      	GV.replaceAllUsesWith(F);
      } else {
        GV.replaceAllUsesWith(Constant::getNullValue(GV.getType()));
      }
      GV.eraseFromParent();
    }
  }

  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E;) {
    GlobalValue& GV = *I;
    ++I;
    if (!strncmp(GV.getName().data(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().data(), "java", 4) ||
        !strncmp(GV.getName().data(), "JnJVM_gnu", 9) ||
        !strncmp(GV.getName().data(), "gnu", 3)) {
      GV.replaceAllUsesWith(Constant::getNullValue(GV.getType()));
      GV.eraseFromParent();
    }
  }
 
  // Replace gcmalloc with the allocator of MMTk objects in VMKit
  Function* F = M.getFunction("gcmalloc");
  Function* Ma = M.getFunction("AllocateMagicArray");

  Function* NewFunction = 
    Function::Create(F->getFunctionType(), GlobalValue::ExternalLinkage,
                     "MMTkMutatorAllocate", &M);

  F->replaceAllUsesWith(NewFunction);
  F->eraseFromParent();
  
  Ma->replaceAllUsesWith(NewFunction);
  Ma->eraseFromParent();

  // Declare two global variables for allocating a MutatorContext object
  // and a CollectorContext object.
  
  const Type* Ty = IntegerType::getInt32Ty(M.getContext());
  GlobalVariable* GV = M.getGlobalVariable("org_j3_config_Selected_4Collector",
                                           false);
  Constant* C = GV->getInitializer();
  C = dyn_cast<Constant>(C->getOperand(1));
  new GlobalVariable(M, Ty, true, GlobalValue::ExternalLinkage,
                     C, "MMTkCollectorSize");


  GV = M.getGlobalVariable("org_j3_config_Selected_4Mutator", false);
  C = GV->getInitializer();
  C = dyn_cast<Constant>(C->getOperand(1));
  new GlobalVariable(M, Ty, true, GlobalValue::ExternalLinkage,
                     C, "MMTkMutatorSize");
  
  
  GV = M.getGlobalVariable("org_mmtk_plan_MutatorContext_VT", false);
  ConstantArray* CA = dyn_cast<ConstantArray>(GV->getInitializer());

  Function* Alloc = M.getFunction("JnJVM_org_mmtk_plan_MutatorContext_alloc__IIIII");
  Function* PostAlloc = M.getFunction("JnJVM_org_mmtk_plan_MutatorContext_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II");
  Function* CheckAlloc = M.getFunction("JnJVM_org_mmtk_plan_MutatorContext_checkAllocator__III");
  uint32_t AllocIndex = 0;
  uint32_t PostAllocIndex = 0;
  uint32_t CheckAllocIndex = 0;
  for (uint32_t i = 0; i < CA->getNumOperands(); ++i) {
    ConstantExpr* CE = dyn_cast<ConstantExpr>(CA->getOperand(i));
    if (CE) {
      C = dyn_cast<Constant>(CE->getOperand(0));
      if (C == Alloc) {
        AllocIndex = i;
      } else if (C == PostAlloc) {
        PostAllocIndex = i;
      } else if (C == CheckAlloc) {
        CheckAllocIndex = i;
      }
    }
  }

  GV = M.getGlobalVariable("org_j3_config_Selected_4Mutator_VT", false);
  CA = dyn_cast<ConstantArray>(GV->getInitializer());

  C = dyn_cast<Constant>(CA->getOperand(AllocIndex));
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkAlloc",
                  C, &M);

  C = dyn_cast<Constant>(CA->getOperand(PostAllocIndex));
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkPostAlloc",
                  C, &M);
  
  C = dyn_cast<Constant>(CA->getOperand(CheckAllocIndex));
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage,
                  "MMTkCheckAllocator", C, &M);
  


  GV = M.getGlobalVariable("org_mmtk_plan_Plan_VT", false);
  CA = dyn_cast<ConstantArray>(GV->getInitializer());

  Function* Boot = M.getFunction("JnJVM_org_mmtk_plan_Plan_boot__");
  Function* PostBoot = M.getFunction("JnJVM_org_mmtk_plan_Plan_postBoot__");
  Function* FullBoot = M.getFunction("JnJVM_org_mmtk_plan_Plan_fullyBooted__");
  Function* Exit = M.getFunction("JnJVM_org_mmtk_plan_Plan_notifyExit__I");
  uint32_t BootIndex = 0;
  uint32_t PostBootIndex = 0;
  uint32_t FullBootIndex = 0;
  uint32_t ExitIndex = 0;
  for (uint32_t i = 0; i < CA->getNumOperands(); ++i) {
    ConstantExpr* CE = dyn_cast<ConstantExpr>(CA->getOperand(i));
    if (CE) {
      C = dyn_cast<Constant>(CE->getOperand(0));
      if (C == Boot) {
        BootIndex = i;
      } else if (C == PostBoot) {
        PostBootIndex = i;
      } else if (C == FullBoot) {
        FullBootIndex = i;
      } else if (C == Exit) {
        ExitIndex = i;
      }
    }
  }

  GV = M.getGlobalVariable("org_j3_config_Selected_4Plan_VT", false);
  assert(GV && "No Plan");
  CA = dyn_cast<ConstantArray>(GV->getInitializer());
  
  C = CA->getOperand(BootIndex);
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkPlanBoot",
                  C, &M);
  
  C = CA->getOperand(PostBootIndex);
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage,
                  "MMTkPlanPostBoot", C, &M);
  
  C = CA->getOperand(FullBootIndex);
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage,
                  "MMTkPlanFullBoot", C, &M);
  
  C = CA->getOperand(ExitIndex);
  C = dyn_cast<Constant>(C->getOperand(0));
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkPlanExit",
                  C, &M);
   
   
   
  return Changed;
}


ModulePass* createLowerJavaRT() {
  return new LowerJavaRT();
}

}
