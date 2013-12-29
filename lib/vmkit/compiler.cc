#include <dlfcn.h>

#include "vmkit/system.h"
#include "vmkit/compiler.h"
#include "vmkit/thread.h"
#include "vmkit/vmkit.h"
#include "vmkit/safepoint.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/PassManager.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/JIT.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "llvm/Target/TargetOptions.h"

using namespace vmkit;

void* Symbol::getSymbolAddress() {
	Thread::get()->vm()->internalError(L"implement me: getSymbolAddress");
}

void* CompilationUnit::operator new(size_t n, BumpAllocator* allocator) {
	return allocator->allocate(n);
}

void  CompilationUnit::operator delete(void* self) {
}

CompilationUnit::CompilationUnit(BumpAllocator* allocator, VMKit* vmkit, const char* id) :
	_symbolTable(vmkit::Util::char_less, allocator) {
	_allocator = allocator;
	pthread_mutex_init(&_mutexSymbolTable, 0);

	_vmkit = vmkit;

	std::string err;

	llvm::TargetOptions opt;
	opt.NoFramePointerElim = 1;

	_ee = llvm::EngineBuilder(new llvm::Module(id, Thread::get()->vm()->llvmContext()))
		.setUseMCJIT(1)
		.setMCJITMemoryManager(this)
		.setOptLevel(llvm::CodeGenOpt::None) /* Aggressive */
		.setTargetOptions(opt)
		.setErrorStr(&err)
		.create();

	if (!ee())
		Thread::get()->vm()->internalError(L"Error while creating execution engine: %s\n", err.c_str());

	ee()->finalizeObject();

	_oldee = llvm::EngineBuilder(new llvm::Module("old ee", Thread::get()->vm()->llvmContext()))
		.setErrorStr(&err)
		.create();

	if (!oldee())
		Thread::get()->vm()->internalError(L"Error while creating execution engine: %s\n", err.c_str());

	oldee()->DisableLazyCompilation(0);

	pm = new llvm::PassManager();
	//pm->add(new llvm::TargetData(*ee->getTargetData()));

#if 0
	pm->add(llvm::createBasicAliasAnalysisPass());
#endif
	
	pm->add(llvm::createCFGSimplificationPass());      // Clean up disgusting code

#if 0
	pm->add(llvm::createPromoteMemoryToRegisterPass());// Kill useless allocas
	pm->add(llvm::createInstructionCombiningPass()); // Cleanup for scalarrepl.
	pm->add(llvm::createScalarReplAggregatesPass()); // Break up aggregate allocas
	pm->add(llvm::createInstructionCombiningPass()); // Cleanup for scalarrepl.
	pm->add(llvm::createJumpThreadingPass());        // Thread jumps.
	pm->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs
	pm->add(llvm::createInstructionCombiningPass()); // Combine silly seq's
	pm->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs

	pm->add(llvm::createReassociatePass());          // Reassociate expressions
	pm->add(llvm::createLoopRotatePass());           // Rotate loops.
	pm->add(llvm::createLICMPass());                 // Hoist loop invariants
	pm->add(llvm::createLoopUnswitchPass());         // Unswitch loops.
	pm->add(llvm::createInstructionCombiningPass());
	pm->add(llvm::createIndVarSimplifyPass());       // Canonicalize indvars
	pm->add(llvm::createLoopDeletionPass());         // Delete dead loops
	pm->add(llvm::createLoopUnrollPass());           // Unroll small loops*/
	pm->add(llvm::createInstructionCombiningPass()); // Clean up after the unroller
	pm->add(llvm::createGVNPass());                  // Remove redundancies
	pm->add(llvm::createMemCpyOptPass());            // Remove memcpy / form memset
	pm->add(llvm::createSCCPPass());                 // Constant prop with SCCP

	// Run instcombine after redundancy elimination to exploit opportunities
	// opened up by them.
	pm->add(llvm::createInstructionCombiningPass());
	pm->add(llvm::createJumpThreadingPass());         // Thread jumps
	pm->add(llvm::createDeadStoreEliminationPass());  // Delete dead stores
	pm->add(llvm::createAggressiveDCEPass());         // Delete dead instructions
	pm->add(llvm::createCFGSimplificationPass());     // Merge & remove BBs
#endif
}

CompilationUnit::~CompilationUnit() {
	delete pm;
	delete _ee;
	delete _oldee;
}

void CompilationUnit::destroy(CompilationUnit* unit) {
	delete unit;
	BumpAllocator::destroy(unit->allocator());
}

void CompilationUnit::addSymbol(const char* id, vmkit::Symbol* symbol) {
	pthread_mutex_lock(&_mutexSymbolTable);
	_symbolTable[id] = symbol;
	pthread_mutex_unlock(&_mutexSymbolTable);
}

Symbol* CompilationUnit::getSymbol(const char* id) {
	pthread_mutex_lock(&_mutexSymbolTable);

	std::map<const char*, vmkit::Symbol*>::iterator it = _symbolTable.find(id);
	vmkit::Symbol* res;

	if(it == _symbolTable.end()) {
		uint8_t* addr = (uint8_t*)dlsym(SELF_HANDLE, id);
		if(!addr)
			Thread::get()->vm()->internalError(L"unable to resolve native symbol: %s", id);
		res = new(allocator()) vmkit::NativeSymbol(addr);
		size_t len = strlen(id);
		char* buf = (char*)allocator()->allocate(len+1);
		memcpy(buf, id, len+1);
		_symbolTable[buf] = res;
	} else
		res = it->second;

	pthread_mutex_unlock(&_mutexSymbolTable);
	return res;
}

uint64_t CompilationUnit::getSymbolAddress(const std::string &Name) {
	return (uint64_t)(uintptr_t)getSymbol(System::mcjitSymbol(Name))->getSymbolAddress();
}

void CompilationUnit::compileModule(llvm::Module* module) {
	pm->run(*module);
	ee()->addModule(module);
	ee()->finalizeObject();

	vmkit::Safepoint* sf = Safepoint::get(this, module);

	if(!sf)
		vm()->internalError(L"unable to find safepoints");
		
	while(sf->addr()) {
		sf->setUnit(this);
		vm()->addSafepoint(sf);

		//vm()->getSafepoint(sf->addr())->dump();
		sf = sf->getNext();
	}
}
