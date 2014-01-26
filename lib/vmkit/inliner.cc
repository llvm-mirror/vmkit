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
	class FunctionInliner {
	public:
		llvm::Function*                          function;
		llvm::SmallPtrSet<llvm::BasicBlock*, 32> visited;
		llvm::SmallVector<std::pair<Symbol*, llvm::BasicBlock*>, 8>  visitStack;
		CompilationUnit*                         originalUnit;
		Symbol*                                  curSymbol;
		bool                                     onlyAlwaysInline;
		uint64_t                                 inlineThreshold;

		FunctionInliner(CompilationUnit* unit, llvm::Function* _function, uint64_t inlineThreshold, bool _onlyAlwaysInline) {
			function = _function;
			originalUnit = unit;
			onlyAlwaysInline = _onlyAlwaysInline;
			push(0, &function->getEntryBlock());
		}

		void push(Symbol* symbol, llvm::BasicBlock* bb) {
			if(visited.insert(bb))
				visitStack.push_back(std::make_pair(symbol, bb));
		}

		llvm::BasicBlock* pop() {
			std::pair<Symbol*, llvm::BasicBlock*> top = visitStack.pop_back_val();
			curSymbol = top.first;
			return top.second;
		}

		Symbol* tryInline(llvm::Function* callee) {
			if(callee->isIntrinsic())
				return 0;

			const char*     id = callee->getName().data();
			CompilationUnit* unit = curSymbol ? curSymbol->unit() : originalUnit;
			if(!unit)
				unit = originalUnit;
			Symbol*         symbol = unit->getSymbol(id, 0);
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
				symbol = new(unit->allocator()) NativeSymbol(callee, addr);
				unit->addSymbol(id, symbol);
			}
		//fprintf(stderr, "       weight: %lld\n", symbol->inlineWeight());

			return (!bc->hasFnAttribute(llvm::Attribute::NoInline)
							&& (bc->hasFnAttribute(llvm::Attribute::AlwaysInline) || 
									(0 && !onlyAlwaysInline && (uint64_t)(symbol->inlineWeight()-1) < inlineThreshold))) ? symbol : 0;
		}

		bool visitBB(llvm::BasicBlock* bb) {
			bool changed = 0;
			bool takeNext = 0;

			//fprintf(stderr, "    visit basic block: %s\n", bb->getName().data());

			for(llvm::BasicBlock::iterator it=bb->begin(), prev=0; it!=bb->end(); takeNext && (prev=it++)) {
				llvm::Instruction *insn = it;
				takeNext = 1;

				//fprintf(stderr, "        visit insn: ");
				//insn->dump();

				//fprintf(stderr, "             %d operands\n", insn->getNumOperands());
				for(unsigned i=0; i<insn->getNumOperands(); i++) {
					llvm::Value* op = insn->getOperand(i);
						
					//fprintf(stderr, " ----> ");
					//op->dump();
					//fprintf(stderr, "     => %s\n", llvm::isa<llvm::GlobalValue>(op) ? "global" : "not global");

					if(llvm::isa<llvm::GlobalValue>(op)) {
						llvm::GlobalValue* gv = llvm::cast<llvm::GlobalValue>(op);
						if(gv->getParent() != function->getParent()) {
							llvm::Value* copy =
								llvm::isa<llvm::Function>(gv) ?
								function->getParent()->getOrInsertFunction(gv->getName().data(), 
																													 llvm::cast<llvm::Function>(gv)->getFunctionType()) :
								function->getParent()->getOrInsertGlobal(gv->getName().data(), gv->getType()->getContainedType(0));

							//fprintf(stderr, "<<<reimporting>>>: %s\n", gv->getName().data());
							gv->replaceAllUsesWith(copy);
						}
					}
				}

				if(insn->getOpcode() != llvm::Instruction::Call &&
					 insn->getOpcode() != llvm::Instruction::Invoke) {
					continue;
				}

				llvm::CallSite  call(insn);
				llvm::Function* callee = call.getCalledFunction();
				
				if(!callee)
					continue;
				
				Symbol* symbol = tryInline(callee);
				
				if(symbol) {
					llvm::Function* bc = symbol->llvmFunction();

					if(bc != callee)
						callee->replaceAllUsesWith(bc);
					
					fprintf(stderr, "            inlining %s in %s\n", bc->getName().data(), function->getName().data());

					if(llvm::isa<llvm::TerminatorInst>(insn)) {
						llvm::TerminatorInst* terminator = llvm::cast<llvm::TerminatorInst>(insn);
						for(unsigned i=0; i<terminator->getNumSuccessors(); i++)
							push(symbol, terminator->getSuccessor(i));
					} else {
						size_t len = strlen(bc->getName().data());
						char buf[len + 16];
						memcpy(buf, bc->getName().data(), len);
						memcpy(buf+len, ".after-inline", 14);
						push(symbol, bb->splitBasicBlock(insn->getNextNode(), buf));
					}
					
					llvm::InlineFunctionInfo ifi(0);
					bool isInlined = llvm::InlineFunction(call, ifi, false);
					//fprintf(stderr, " ok?: %d\n", isInlined);
					changed |= isInlined;

					if(isInlined) {
						curSymbol = symbol;
						if(prev)
							it = prev;
						else {
							takeNext = 0;
							it = bb->begin();
						}
					} else if(bc != callee)
						bc->replaceAllUsesWith(callee);
				}
			}

			return changed;
		}

		bool proceed() {
			bool changed = 0;

			//fprintf(stderr, "visit function: %s\n", function->getName().data());

			while(visitStack.size()) {
				llvm::BasicBlock* bb = pop();

				changed |= visitBB(bb);

				llvm::TerminatorInst* terminator = bb->getTerminator();
				if(terminator) {
					for(unsigned i=0; i<terminator->getNumSuccessors(); i++)
						push(curSymbol, terminator->getSuccessor(i));
				}
			}

			if(changed) {
				//function->dump();
			}

			return changed;
		}
	};

  class FunctionInlinerPass : public llvm::FunctionPass {
  public:
    static char ID;

		CompilationUnit*         unit;
		llvm::InlineCostAnalysis costAnalysis;
		unsigned int             inlineThreshold; 		// 225 in llvm
		bool                     onlyAlwaysInline;

		FunctionInlinerPass(CompilationUnit* _unit, unsigned int _inlineThreshold, bool _onlyAlwaysInline) : 
			FunctionPass(ID) { 
			unit = _unit;
			inlineThreshold = _inlineThreshold; 
			onlyAlwaysInline = _onlyAlwaysInline;
		}

    virtual const char* getPassName() const { return "VMKit inliner"; }
		bool                ensureLocal(llvm::Function* function, llvm::Function* callee);
		Symbol*             tryInline(llvm::Function* function, llvm::Function* callee);
		bool                runOnBB(llvm::BasicBlock* bb);
		bool                runOnFunction0(llvm::Function& function);
		bool                runOnFunction(llvm::Function& function) {
#if 0
			return runOnFunction0(function);
#else
			FunctionInliner inliner(unit, &function, inlineThreshold, onlyAlwaysInline);
			return inliner.proceed();
#endif
		}
	};

  char FunctionInlinerPass::ID = 0;

#if 0
	llvm::RegisterPass<FunctionInlinerPass> X("FunctionInlinerPass",
																				"Inlining Pass that inlines evaluator's functions.");
#endif

	//FunctionInlinerPass() : FunctionPass(ID) {}

	bool FunctionInlinerPass::ensureLocal(llvm::Function* function, llvm::Function* callee) {
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
		
	//llvm::SmallPtrSet<const Function*, 16> NeverInline;

	bool FunctionInlinerPass::runOnBB(llvm::BasicBlock* bb) {
		fprintf(stderr, " process basic block %s\n", bb->getName().data());

			//SmallPtrSet<const BasicBlock*, 8> Visited;

		return 0;
	}

	bool FunctionInlinerPass::runOnFunction0(llvm::Function& function) {
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

#if 0
	llvm::RegisterPass<FunctionInlinerPass> X("FunctionInlinerPass",
																				"Inlining Pass that inlines evaluator's functions.");
#endif

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler, bool onlyAlwaysInline) {
		return new FunctionInlinerPass(compiler, 2000, onlyAlwaysInline);
	}
}
