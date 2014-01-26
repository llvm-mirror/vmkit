#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "vmkit/compiler.h"
#include "vmkit/system.h"
#include "vmkit/vmkit.h"

#include <dlfcn.h>

namespace vmkit {
  class FunctionInliner : public llvm::FunctionPass {
  public:
    static char ID;

		CompilationUnit*         compiler;
		llvm::InlineCostAnalysis costAnalysis;
		unsigned int             inlineThreshold; 		// 225 in llvm
		bool                     onlyAlwaysInline;

		//FunctionInliner() : FunctionPass(ID) {}
    FunctionInliner(CompilationUnit* _compiler, unsigned int _inlineThreshold, bool _onlyAlwaysInline) : 
			FunctionPass(ID) { 
			compiler = _compiler;
			inlineThreshold = _inlineThreshold; 
			onlyAlwaysInline = _onlyAlwaysInline;
		}

    virtual const char* getPassName() const {
      return "Simple inliner";
    }

		bool ensureLocal(llvm::Function* function, llvm::Function* callee) {
			/* prevent exernal references because some llvm passes don't like that */
			if(callee->getParent() != function->getParent()) {
				//fprintf(stderr, "       rewrite local\n");
				llvm::Function* local = (llvm::Function*)function->getParent()->getOrInsertFunction(callee->getName().data(), 
																																														callee->getFunctionType());
				callee->replaceAllUsesWith(local);
				callee = local;
				return 1;
			} else
				return 0;
		}
		
		Symbol* tryInline(llvm::Function* function, llvm::Function* callee) {
			if(callee->isIntrinsic())
				return 0;

			const char*     id = callee->getName().data();
			Symbol*         symbol = compiler->getSymbol(id, 0);
			llvm::Function* bc;

			//fprintf(stderr, "   processing: %s => %p\n", id, symbol);

			if(symbol) {
				bc = symbol->llvmFunction();
				if(!bc)
					return 0;
			} else {
				bc = callee;

				if(callee->isDeclaration() && callee->isMaterializable())
					callee->Materialize();

				if(callee->isDeclaration())
					return 0;
					
				uint8_t* addr = (uint8_t*)dlsym(SELF_HANDLE, id);
				symbol = new(compiler->allocator()) NativeSymbol(callee, addr);
				compiler->addSymbol(id, symbol);
			}

			//fprintf(stderr, "       weight: %lld\n", symbol->inlineWeight());

			return (!bc->hasFnAttribute(llvm::Attribute::NoInline)
							&& (bc->hasFnAttribute(llvm::Attribute::AlwaysInline) || 
									(!onlyAlwaysInline && (uint64_t)(symbol->inlineWeight()-1) < inlineThreshold))) ? symbol : 0;
		}

		//llvm::SmallPtrSet<const Function*, 16> NeverInline;

		bool runOnFunction(llvm::Function& function) {
			bool     changed = false;
			
			//fprintf(stderr, "Analyzing: %s\n", function.getName().data());
			
		restart:
			for (llvm::Function::iterator bit=function.begin(); bit!=function.end(); bit++) { 
				llvm::BasicBlock* bb = bit; 
				uint32_t prof = 0;

				for(llvm::BasicBlock::iterator it=bb->begin(), prev=0; it!=bb->end() && prof<42; prev=it++) {
					llvm::Instruction *insn = it;

					//fprintf(stderr, "  process: ");
					//insn->dump();

#if 0
					if(insn->getOpcode() == llvm::Instruction::LandingPad) {
						llvm::LandingPadInst* lp = (llvm::LandingPadInst*)insn;
						ensureLocal(&function, (llvm::Function*)lp->getPersonalityFn());
						continue;
					}
#endif

					if (insn->getOpcode() != llvm::Instruction::Call &&
							insn->getOpcode() != llvm::Instruction::Invoke) {
						continue;
					}

					llvm::CallSite  call(insn);
					llvm::Function* callee = call.getCalledFunction();

					if(!callee)
						continue;

					Symbol* symbol = tryInline(&function, callee);
					llvm::Function* bc;

					if(symbol && (bc = symbol->llvmFunction()) != &function) {
						if(bc != callee)
							callee->replaceAllUsesWith(bc);

						//fprintf(stderr, "   inlining: %s\n", bc->getName().data());
						//bc->dump();
						llvm::InlineFunctionInfo ifi(0);
						bool isInlined = llvm::InlineFunction(call, ifi, false);
						changed |= isInlined;

						if(isInlined) {
							prof++;
							//it = prev ? prev : bb->begin();
							//continue;
							//fprintf(stderr, "... restart ....\n");
							goto restart;
						}
					} else
						changed |= ensureLocal(&function, callee);
				}
			}

			//#if 0
			if(changed) {
				//function.dump();
				//abort();
			}
			//#endif

			return changed;
		}
	};

  char FunctionInliner::ID = 0;

#if 0
	llvm::RegisterPass<FunctionInliner> X("FunctionInliner",
																				"Inlining Pass that inlines evaluator's functions.");
#endif

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler, bool onlyAlwaysInline) {
		return new FunctionInliner(compiler, 2000, onlyAlwaysInline);
	}
}
