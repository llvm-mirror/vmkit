#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
//#include "llvm/Target/TargetData.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "vmkit/compiler.h"

namespace vmkit {
  class FunctionInliner : public llvm::FunctionPass {
  public:
    static char ID;

		CompilationUnit*                             compiler;
		llvm::InlineCostAnalysis                     costAnalysis;
		unsigned int                                 inlineThreshold;

		//FunctionInliner() : FunctionPass(ID) {}
    FunctionInliner(CompilationUnit* _compiler, unsigned int _inlineThreshold=225) : FunctionPass(ID) { 
			compiler = _compiler;
			inlineThreshold = _inlineThreshold; 
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
		bool Changed = false;

		for (llvm::Function::iterator bit=function.begin(); bit!=function.end(); bit++) { 
			llvm::BasicBlock* bb = bit; 

			for(llvm::BasicBlock::iterator it=bb->begin(); it!=bb->end();) {
				llvm::Instruction *insn = it++;

				if (insn->getOpcode() != llvm::Instruction::Call &&
						insn->getOpcode() != llvm::Instruction::Invoke) {
					continue;
				}

				llvm::CallSite  call(insn);
				llvm::Function* original = call.getCalledFunction();
				llvm::Function* callee = original;

				if(!callee)
					continue;

				if(callee->isDeclaration()) { /* ok, resolve */
					Symbol* s = compiler->getSymbol(callee->getName().data(), 0);
					
					if(s && s->isInlinable())
						callee = s->llvmFunction();
				}

				if(callee && !callee->isDeclaration()) {
					llvm::InlineCost cost = getInlineCost(call, callee);
					if(cost.isAlways()) {// || (!cost.isNever() && (cost))) {
						//fprintf(stderr, "----   Inlining: %s\n", callee->getName().data());

						llvm::InlineFunctionInfo ifi(0);
						bool isInlined = llvm::InlineFunction(call, ifi, false);
						Changed |= isInlined;

						if(isInlined){
							it = bb->begin();
							continue;
						}
					}
				}

				if(original->getParent() != function.getParent()) {
					callee = (llvm::Function*)function.getParent()->getOrInsertFunction(original->getName(), original->getFunctionType());
					original->replaceAllUsesWith(callee);
					Changed = 1;
				}
			}
		}

		return Changed;
	}

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler) {
		return new FunctionInliner(compiler);
	}
}
