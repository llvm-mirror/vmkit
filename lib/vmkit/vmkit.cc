#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "vmkit/vmkit.h"
#include "vmkit/thread.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/PassManager.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Intrinsics.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/MemoryBuffer.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/CodeGen/MachineCodeEmitter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/GCStrategy.h"

using namespace vmkit;

VMKit::VMKit(vmkit::BumpAllocator* allocator) :
	mangleMap(Util::char_less, allocator) {
	llvm::InitializeNativeTarget();
	_allocator = allocator;
}

void* VMKit::operator new(size_t n, vmkit::BumpAllocator* allocator) {
	return allocator->allocate(n);
}

void VMKit::destroy(VMKit* vm) {
	vmkit::BumpAllocator::destroy(vm->allocator());
}


llvm::Type* VMKit::introspectType(const char* name) {
	llvm::Type* res = self()->getTypeByName(name);
	if(!res)
		internalError(L"unable to find internal type: %s", name);
	return res;
}

llvm::Function* VMKit::introspectFunction(llvm::Module* dest, const char* name) {
	llvm::Function* orig = (llvm::Function*)mangleMap[name];
	if(!orig)
		internalError(L"unable to find internal function: %s", name);

	return orig;
	//	return (llvm::Function*)dest->getOrInsertFunction(orig->getName(), orig->getFunctionType());
}

llvm::GlobalValue* VMKit::introspectGlobalValue(llvm::Module* dest, const char* name) {
	llvm::GlobalValue* res = mangleMap[name];
	if(!res)
		internalError(L"unable to find internal global value: %s", name);
	return res;
}

uintptr_t VMKit::addSymbol(llvm::GlobalValue* gv) {
	const char* id = gv->getName().data();
	void* ptr = dlsym(RTLD_SELF, id);

	if(ptr) {
		ee()->updateGlobalMapping(gv, ptr);
		int   status;
		char* realname;
		realname = abi::__cxa_demangle(id, 0, 0, &status);
		const char* tmp = realname ? realname : id;
		uint32_t length = strlen(tmp);
		char* mangled = (char*)allocator()->allocate(length+1);
		strcpy(mangled, tmp);
		mangleMap[mangled] = gv;
		free(realname);
		return (uintptr_t)ptr;
	} else
		return 0;
}

void VMKit::vmkitBootstrap(Thread* initialThread, const char* selfBitCodePath) {
	Thread::set(initialThread);

	std::string err;
	llvm::OwningPtr<llvm::MemoryBuffer> buf;
	if (llvm::MemoryBuffer::getFile(selfBitCodePath, buf))
		VMKit::internalError(L"Error while opening bitcode file %s\n", selfBitCodePath);
	_self = llvm::getLazyBitcodeModule(buf.take(), llvm::getGlobalContext(), &err);

	if(!self())
		VMKit::internalError(L"Error while reading bitcode file %s: %s\n", selfBitCodePath, err.c_str());

	_ee = llvm::EngineBuilder(self()).setErrorStr(&err).create();
	if (!ee())
		VMKit::internalError(L"Error while creating execution engine: %s\n", err.c_str());

	ee()->DisableLazyCompilation(0);
  ee()->RegisterJITEventListener(this);

	for(llvm::Module::iterator cur=self()->begin(); cur!=self()->end(); cur++)
		addSymbol(cur);

	for(llvm::Module::global_iterator cur=self()->global_begin(); cur!=self()->global_end(); cur++)
		addSymbol(cur);

	_dataLayout = new llvm::DataLayout(self());

	ptrTypeInfo = ee()->getPointerToGlobal(introspectGlobalValue(self(), "typeinfo for void*"));

	if(!ptrTypeInfo)
		internalError(L"unable to find typeinfo for void*"); 

#if 0
	llvm::Linker* linker = new llvm::Linker(new llvm::Module("linker", vm()->self()->getContext()));
	std::string err;
	if(linker->linkInModule(vm()->self(), llvm::Linker::PreserveSource, &err))
		J3::internalError(L"unable to add self to linker: %s", err.c_str());
	if(linker->linkInModule(module(), llvm::Linker::PreserveSource, &err))
		J3::internalError(L"unable to add module to linker: %s", err.c_str());
#endif
}


llvm::Function* VMKit::getGCRoot(llvm::Module* mod) {
	return llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::gcroot);
}

llvm::FunctionPassManager* VMKit::preparePM(llvm::Module* mod) {
	llvm::FunctionPassManager* pm = new llvm::FunctionPassManager(mod);
	//pm->add(new llvm::TargetData(*ee->getTargetData()));

	pm->add(llvm::createBasicAliasAnalysisPass());
	
	pm->add(llvm::createCFGSimplificationPass());      // Clean up disgusting code
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

	pm->doInitialization();

	return pm;
}

void VMKit::NotifyFunctionEmitted(const llvm::Function &F,
																	void *Code,
																	size_t Size,
																	const llvm::JITEventListener::EmittedFunctionDetails &Details) {
	const llvm::MachineFunction*             mf = Details.MF;
	const std::vector<llvm::LandingPadInfo>& landingPads = mf->getMMI().getLandingPads();
	const llvm::MachineCodeEmitter*          mce = Details.MCE;

	for(std::vector<llvm::LandingPadInfo>::const_iterator i=landingPads.begin(); i!=landingPads.end(); i++) {
		uintptr_t dest = mce->getLabelAddress(i->LandingPadLabel);
		
		for(uint32_t j=0; j<i->BeginLabels.size(); j++) {
			uintptr_t point = mce->getLabelAddress(i->EndLabels[j]);
			ExceptionDescriptor* e = new ExceptionDescriptor(&F, point, dest);
			exceptionTable[point] = e;
			fprintf(stderr, "      exceptionpoint at 0x%lx goes to 0x%lx\n", point, dest);
		}
	}

	if(F.hasGC()) {
		llvm::GCFunctionInfo* gcInfo = &mf->getGMI()->getFunctionInfo(F);
		uintptr_t start = (uintptr_t)Code;
		uintptr_t end = start + Size;

		for(llvm::GCFunctionInfo::iterator safepoint=gcInfo->begin(); safepoint!=gcInfo->end(); safepoint++) {
			uint32_t kind = safepoint->Kind;
			llvm::MCSymbol* label = safepoint->Label;
			uintptr_t addr = mce->getLabelAddress(label);

			if(addr < start || addr > end)
				internalError(L"safe point is not inside the function (%p %p %p)", start, addr, end);

			Safepoint* sf = Safepoint::create(&F, addr, gcInfo->live_size(safepoint));
			uint32_t i=0;

			//fprintf(stderr, "      safepoint at 0x%lx\n", addr);
			for(llvm::GCFunctionInfo::live_iterator live=gcInfo->live_begin(safepoint); live!=gcInfo->live_end(safepoint); live++, i++) {
				sf->setAt(i, live->StackOffset);
				//fprintf(stderr, "       offset: %d\n", live->StackOffset); 
			}
			safepointMap[addr] = sf;
		}
	}
}

void VMKit::log(const wchar_t* msg, ...) {
	va_list va;
	va_start(va, msg);
	fprintf(stderr, "[vmkit]: ");
	vfwprintf(stderr, msg, va);
	fprintf(stderr, "\n");
	va_end(va);
}

void VMKit::internalError(const wchar_t* msg, va_list va) {
	defaultInternalError(msg, va);
}

void VMKit::defaultInternalError(const wchar_t* msg, va_list va) {
	fprintf(stderr, "Fatal error: ");
	vfwprintf(stderr, msg, va);
	fprintf(stderr, "\n");
	abort();
}

void VMKit::internalError(const wchar_t* msg, ...) {
	va_list va;
	va_start(va, msg);
	if(Thread::get() && Thread::get()->vm())
		Thread::get()->vm()->internalError(msg, va);
	else
		defaultInternalError(msg, va);
	va_end(va);
	fprintf(stderr, "SHOULD NOT BE THERE\n");
	abort();
}

void VMKit::throwException(void* obj) {
	void** exception = (void**)abi::__cxa_allocate_exception(sizeof(void*));
	*exception = obj;
	abi::__cxa_throw(exception, (std::type_info*)Thread::get()->vm()->ptrTypeInfo, 0);
	abort();
}


void* Safepoint::operator new(size_t unused, size_t nbSlots) {
	return ::operator new(sizeof(Safepoint) + nbSlots*sizeof(uintptr_t) - sizeof(uintptr_t));
}

Safepoint::Safepoint(const llvm::Function* llvmFunction, uintptr_t address, size_t nbSlots) {
	_llvmFunction = llvmFunction;
	_address = address;
	_nbSlots = nbSlots;
}

Safepoint* Safepoint::create(const llvm::Function* llvmFunction, uintptr_t address, size_t nbSlots) {
	return new(nbSlots) Safepoint(llvmFunction, address, nbSlots);
}

ExceptionDescriptor::ExceptionDescriptor(const llvm::Function* llvmFunction, uintptr_t point, uintptr_t landingPad) {
	_llvmFunction = llvmFunction;
	_point = point;
	_landingPad = landingPad;
}
