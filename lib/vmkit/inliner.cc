#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "vmkit/compiler.h"

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

		llvm::InlineCost getInlineCost(llvm::CallSite callSite, llvm::Function* callee) {
			return costAnalysis.getInlineCost(callSite, inlineThreshold);
		}

    virtual bool runOnFunction(llvm::Function &function);
  private:
  };

  char FunctionInliner::ID = 0;

#if 0
	llvm::RegisterPass<FunctionInliner> X("FunctionInliner",
																				"Inlining Pass that inlines evaluator's functions.");
#endif

	bool FunctionInliner::runOnFunction(llvm::Function& function) {
		bool     Changed = false;

		//fprintf(stderr, "Analyzing: %s\n", function.getName().data());

	restart:
		for (llvm::Function::iterator bit=function.begin(); bit!=function.end(); bit++) { 
			llvm::BasicBlock* bb = bit; 
			uint32_t prof = 0;

			for(llvm::BasicBlock::iterator it=bb->begin(), prev=0; it!=bb->end() && prof<42; prev=it++) {
				llvm::Instruction *insn = it;

				//fprintf(stderr, "  process: ");
				//insn->dump();

				if (insn->getOpcode() != llvm::Instruction::Call &&
						insn->getOpcode() != llvm::Instruction::Invoke) {
					continue;
				}

				llvm::CallSite  call(insn);
				llvm::Function* callee = call.getCalledFunction();

				if(!callee)
					continue;

				llvm::Function* bc = callee;

				if(callee->isDeclaration()) { /* ok, resolve */
					if(callee->isMaterializable())
						callee->Materialize();

					if(callee->isDeclaration()) {
						Symbol* s = compiler->getSymbol(bc->getName().data(), 0);
					
						if(s && s->isInlinable())
							bc = s->llvmFunction();
					}
				}

				/* getInlineCost does not like inter-module references */
				if(callee->getParent() != function.getParent()) {
					llvm::Function* local = (llvm::Function*)function.getParent()->getOrInsertFunction(callee->getName(), 
																																															 callee->getFunctionType());
					callee->replaceAllUsesWith(local);
					callee = local;
					Changed = 1;
				}

				if(bc && !bc->isDeclaration()) {
					//fprintf(stderr, " processing %s\n", bc->getName().data());
					//function.dump();
					//fprintf(stderr, " %p and %p\n", function.getParent(), callee->getParent());
					//callee->dump();

					llvm::InlineCost cost = getInlineCost(call, bc);

					//fprintf(stderr, "      Inlining: %s ", bc->getName().data());
					//if(cost.isAlways())
					//fprintf(stderr, " is always\n");
					//else if(cost.isNever())
					//fprintf(stderr, " is never\n");
					//else 
					//fprintf(stderr, " cost: %d (=> %s)\n", cost.getCost(), !cost ? "false" : "true");

					if(cost.isAlways()) {// || (!onlyAlwaysInline && !cost.isNever() && cost)) {
						if(bc != callee)
							callee->replaceAllUsesWith(bc);
						
						llvm::InlineFunctionInfo ifi(0);
						bool isInlined = llvm::InlineFunction(call, ifi, false);
						Changed |= isInlined;

						if(isInlined) {
							//							prof++;
							//							it = prev ? prev : bb->begin();
							//							continue;
							goto restart;
						}
					}
				}
			}
		}

		//function.dump();

		return Changed;
	}

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler, bool onlyAlwaysInline) {
		return new FunctionInliner(compiler, 2, onlyAlwaysInline);
	}
}
