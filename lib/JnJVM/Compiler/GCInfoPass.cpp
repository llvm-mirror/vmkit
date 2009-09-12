//===---- GCInfoPass.cpp - Get GC info for JIT-generated functions --------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/Support/Compiler.h"

#include "jnjvm/JnjvmModule.h"

using namespace jnjvm;
using namespace llvm;

namespace {
  class VISIBILITY_HIDDEN GCInfoPass : public FunctionPass {
  public:
    static char ID;
    JavaLLVMCompiler* Comp;
    GCInfoPass(JavaLLVMCompiler* C) : FunctionPass(&ID) {
      Comp = C;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const {
      FunctionPass::getAnalysisUsage(AU);      
      AU.setPreservesAll();
      AU.addRequired<GCModuleInfo>();
    }

    bool runOnFunction(Function &F) {
      // Quick exit for functions that do not use GC.
      assert(F.hasGC() && "Function without GC");
      GCFunctionInfo &FI = getAnalysis<GCModuleInfo>().getFunctionInfo(F);
      JavaMethod* meth = Comp->getJavaMethod(&F);
      assert(meth && "Function without a Java method");
      
      LLVMMethodInfo* LMI = Comp->getMethodInfo(meth);
      LMI->GCInfo = &FI;
      return false;
    }
  };
}

char GCInfoPass::ID = 0;

namespace jnjvm {
  FunctionPass* createGCInfo(JavaLLVMCompiler* C) {
    return new GCInfoPass(C);
  }
}

