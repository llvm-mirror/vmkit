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

#include "jnjvm/JnjvmModule.h"


using namespace llvm;

namespace {

  class VISIBILITY_HIDDEN LowerJavaRT : public ModulePass {
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
    GlobalValue& GV = *I;
    ++I;
    if (!strncmp(GV.getName().data(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().data(), "java", 4)) {
      GV.replaceAllUsesWith(Constant::getNullValue(GV.getType()));
      GV.eraseFromParent();
    }
  }

  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E;) {
    GlobalValue& GV = *I;
    ++I;
    if (!strncmp(GV.getName().data(), "JnJVM_java", 10) ||
        !strncmp(GV.getName().data(), "java", 4)) {
      GV.replaceAllUsesWith(Constant::getNullValue(GV.getType()));
      GV.eraseFromParent();
    }
  }
 
  // Replace gcmalloc with the allocator of MMTk objects in VMKit
  Function* F = M.getFunction("gcmalloc");

  Function* NewFunction = 
    Function::Create(F->getFunctionType(), GlobalValue::ExternalLinkage,
                     "MMTkMutatorAllocate", &M);

  F->replaceAllUsesWith(NewFunction);
  F->eraseFromParent();

  // Declare two global variables for allocating a MutatorContext object
  // and a CollectorContext object.
  
  const Type* Ty = IntegerType::getInt32Ty(M.getContext());
  GlobalVariable* GV = M.getGlobalVariable("org_j3_config_Selected_4Collector",
                                           false);
  Constant* C = GV->getInitializer();
  C = C->getOperand(1);
  new GlobalVariable(M, Ty, true, GlobalValue::ExternalLinkage,
                     C, "MMTkCollectorSize");


  GV = M.getGlobalVariable("org_j3_config_Selected_4Mutator", false);
  C = GV->getInitializer();
  C = C->getOperand(1);
  new GlobalVariable(M, Ty, true, GlobalValue::ExternalLinkage,
                     C, "MMTkMutatorSize");
  
  
  GV = M.getGlobalVariable("org_mmtk_plan_MutatorContext_VT", false);
  ConstantArray* CA = dyn_cast<ConstantArray>(GV->getInitializer());

  Function* Alloc = M.getFunction("JnJVM_org_mmtk_plan_MutatorContext_alloc__IIIII");
  Function* PostAlloc = M.getFunction("JnJVM_org_mmtk_plan_MutatorContext_postAlloc__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_ObjectReference_2II");
  uint32_t AllocIndex = 0;
  uint32_t PostAllocIndex = 0;
  for (uint32_t i = 0; i < CA->getNumOperands(); ++i) {
    ConstantExpr* CE = dyn_cast<ConstantExpr>(CA->getOperand(i));
    if (CE) {
      C = CE->getOperand(0);
      if (C == Alloc) {
        AllocIndex = i;
      } else if (C == PostAlloc) {
        PostAllocIndex = i;
      }
    }
  }

  GV = M.getGlobalVariable("org_j3_config_Selected_4Mutator_VT", false);
  CA = dyn_cast<ConstantArray>(GV->getInitializer());

  C = CA->getOperand(AllocIndex);
  C = C->getOperand(0);
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkAlloc",
                  C, &M);

  C = CA->getOperand(PostAllocIndex);
  C = C->getOperand(0);
  new GlobalAlias(C->getType(), GlobalValue::ExternalLinkage, "MMTkPostAlloc",
                  C, &M);
   
  return Changed;
}


ModulePass* createLowerJavaRT() {
  return new LowerJavaRT();
}

}
