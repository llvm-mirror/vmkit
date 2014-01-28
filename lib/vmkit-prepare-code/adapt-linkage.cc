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
  class VMKitAdaptLinkage : public llvm::ModulePass {
  public:
    static char ID;
    
    VMKitAdaptLinkage() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &module);
	};

  char VMKitAdaptLinkage::ID = 0;
	llvm::RegisterPass<VMKitAdaptLinkage> X("VMKitAdaptLinkage",
																					"Adapt the linkage for vmkit");

	class VMKitRenamer  {
	public:
		size_t moduleLen;
		size_t reserved;
		char*  buf;

		VMKitRenamer(llvm::Module* module) {
			const char* moduleId = module->getModuleIdentifier().data();
			moduleLen = strlen(moduleId);
			reserved = 1024;
			buf = (char*)malloc(moduleLen + reserved + 1);
			memcpy(buf, moduleId, moduleLen);
		}

		bool makeVisible(llvm::GlobalValue* gv) {
			llvm::GlobalValue::LinkageTypes linkage = gv->getLinkage();

			if(linkage == llvm::GlobalValue::LinkOnceODRLinkage) {
				gv->setLinkage(llvm::GlobalValue::WeakODRLinkage);
				return 1;
			}

			if(linkage == llvm::GlobalValue::LinkOnceAnyLinkage) {
				gv->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
				return 1;
			}


			if(linkage == llvm::GlobalValue::PrivateLinkage && !memcmp(gv->getName().data(), ".str", 4)) {
				size_t len = strlen(gv->getName().data());
				if(len > reserved) {
					reserved = (len << 2);
					buf = (char*)realloc(buf, moduleLen + reserved + 1);
				}
				memcpy(buf + moduleLen, gv->getName().data(), len + 1);
				gv->setName(buf);
				gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
				return 1;
			} 

			return 0;
		}
	};

	bool VMKitAdaptLinkage::runOnModule(llvm::Module &module) {
		bool changed = 0;
		VMKitRenamer renamer(&module);

		for (llvm::Module::iterator it=module.begin(); it!=module.end(); it++)
			changed |= renamer.makeVisible(it);

		for (llvm::Module::global_iterator it=module.global_begin(); it!=module.global_end(); it++)
			changed |= renamer.makeVisible(it);

		return changed;
  };
}
