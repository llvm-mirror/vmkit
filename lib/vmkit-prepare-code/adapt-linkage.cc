//===---- StaticGCPass.cpp - Put GC information in functions compiled --------//
//===----------------------- with llvm-gcc --------------------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>

namespace vmkit {
  class VMKitAdaptLinkage : public llvm::FunctionPass {
  public:
    static char ID;
    
    VMKitAdaptLinkage() : llvm::FunctionPass(ID) {}

    virtual bool runOnFunction(llvm::Function& function) { 
			llvm::Function::LinkageTypes linkage = function.getLinkage();

			if(linkage == llvm::GlobalValue::LinkOnceODRLinkage)
				function.setLinkage(llvm::GlobalValue::WeakODRLinkage);
			if(linkage == llvm::GlobalValue::LinkOnceAnyLinkage)
				function.setLinkage(llvm::GlobalValue::WeakAnyLinkage);
			//if(linkage
			//fprintf(stderr, "run on function: %s\n", function.getName().data());
			return 0;
		}
  };

  char VMKitAdaptLinkage::ID = 0;
	llvm::RegisterPass<VMKitAdaptLinkage> X("VMKitAdaptLinkage",
																					"Adapt the linkage for vmkit");
}
