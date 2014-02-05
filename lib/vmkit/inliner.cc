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
	class InlineBB {
	public:
		Symbol*           symbol;
		llvm::BasicBlock* bb;
		uint64_t          threshold;

		InlineBB(Symbol* _symbol, llvm::BasicBlock* _bb, uint64_t _threshold) {
			symbol = _symbol;
			bb = _bb;
			threshold = _threshold;
		}
	};

	class FunctionInliner {
	public:
		llvm::Function*                          function;
		llvm::SmallPtrSet<llvm::BasicBlock*, 32> visited;
		llvm::SmallVector<InlineBB, 8>           visitStack;
		CompilationUnit*                         originalUnit;
		Symbol*                                  curSymbol;
		bool                                     onlyAlwaysInline;
		uint64_t                                 inlineThreshold;

		FunctionInliner(CompilationUnit* unit, llvm::Function* _function, uint64_t _inlineThreshold, bool _onlyAlwaysInline) {
			function = _function;
			originalUnit = unit;
			onlyAlwaysInline = _onlyAlwaysInline;
			inlineThreshold = _inlineThreshold;
			push(0, &function->getEntryBlock());
		}

		void push(Symbol* symbol, llvm::BasicBlock* bb) {
			if(visited.insert(bb))
				visitStack.push_back(InlineBB(symbol, bb, inlineThreshold));
		}

		llvm::BasicBlock* pop() {
			InlineBB top = visitStack.pop_back_val();
			curSymbol = top.symbol;
			inlineThreshold = top.threshold;
			return top.bb;
		}

		Symbol* tryInline(llvm::Function* callee, uint64_t* weight) {
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
			
			if(bc->hasFnAttribute(llvm::Attribute::NoInline))
				return 0;
			if(bc->hasFnAttribute(llvm::Attribute::AlwaysInline))
				return symbol;
			if(onlyAlwaysInline)
				return 0;

			*weight = symbol->inlineWeight();
			if(*weight == -1)
				return 0;

			//fprintf(stderr, "       %s weight: %lld/%lld\n", bc->getName().data(), *weight, inlineThreshold);

			return *weight < inlineThreshold ? symbol : 0;
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

							if(curSymbol && curSymbol->unit()) {
								Symbol* remoteSymbol = curSymbol->unit()->getSymbol(gv->getName().data(), 0);
								if(remoteSymbol)
									originalUnit->addSymbol(gv->getName().data(), remoteSymbol);
							}

							//fprintf(stderr, "<<<reimporting>>>: %s\n", gv->getName().data());
							insn->setOperand(i, copy);
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

				uint64_t weight;
				Symbol* symbol = tryInline(callee, &weight);
				
				if(symbol) {
					llvm::Function* bc = symbol->llvmFunction();
					
					//fprintf(stderr, "            inlining %s in %s\n", bc->getName().data(), function->getName().data());

					if(llvm::isa<llvm::TerminatorInst>(insn)) {
						llvm::TerminatorInst* terminator = llvm::cast<llvm::TerminatorInst>(insn);
						for(unsigned i=0; i<terminator->getNumSuccessors(); i++)
							push(curSymbol, terminator->getSuccessor(i));
					} else {
						size_t len = strlen(bc->getName().data());
						char buf[len + 14];
						memcpy(buf, bc->getName().data(), len);
						memcpy(buf+len, ".after-inline", 14);
						push(curSymbol, bb->splitBasicBlock(insn->getNextNode(), buf));
					}

					if(bc != callee)
						call.setCalledFunction(bc);
					
					llvm::InlineFunctionInfo ifi(0);
					bool isInlined = llvm::InlineFunction(call, ifi, false);
					//fprintf(stderr, " ok?: %d\n", isInlined);
					changed |= isInlined;

					if(isInlined) {
						curSymbol = symbol;
						inlineThreshold -= weight;
						if(prev)
							it = prev;
						else {
							takeNext = 0;
							it = bb->begin();
						}
					} else {
						symbol->markAsNeverInline();
						if(bc != callee)
							call.setCalledFunction(callee);
					}
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
		bool                runOnFunction(llvm::Function& function) {
			FunctionInliner inliner(unit, &function, inlineThreshold, onlyAlwaysInline);
			return inliner.proceed();
		}
	};

  char FunctionInlinerPass::ID = 0;

#if 0
	llvm::RegisterPass<FunctionInlinerPass> X("FunctionInlinerPass",
																				"Inlining Pass that inlines evaluator's functions.");
#endif

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler, bool onlyAlwaysInline) {
		return new FunctionInlinerPass(compiler, 50*256, onlyAlwaysInline); /* aka 50 llvm instructions */
	}
}
