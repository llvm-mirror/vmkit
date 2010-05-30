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

  return Changed;
}


ModulePass* createLowerJavaRT() {
  return new LowerJavaRT();
}

}
