#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "vmkit/vmkit.h"
#include "vmkit/thread.h"
#include "vmkit/safepoint.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/DataLayout.h"

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
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetDisassembler();
	llvm::llvm_start_multithreaded();
	_allocator = allocator;
	pthread_mutex_init(&safepointMapLock, 0);
}

void* VMKit::operator new(size_t n, vmkit::BumpAllocator* allocator) {
	return allocator->allocate(n);
}

void VMKit::destroy(VMKit* vm) {
	vmkit::BumpAllocator::destroy(vm->allocator());
}

void VMKit::addSafepoint(Safepoint* sf) {
	pthread_mutex_lock(&safepointMapLock);
	safepointMap[sf->addr()] = sf;
	pthread_mutex_unlock(&safepointMapLock);
}

Safepoint* VMKit::getSafepoint(uintptr_t addr) {
	pthread_mutex_lock(&safepointMapLock);
	Safepoint* res = safepointMap[addr];
	pthread_mutex_unlock(&safepointMapLock);
	return res;
}

llvm::LLVMContext& VMKit::llvmContext() {
	return self()->getContext();
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

	return (llvm::Function*)dest->getOrInsertFunction(orig->getName(), orig->getFunctionType());
}

llvm::GlobalValue* VMKit::introspectGlobalValue(llvm::Module* dest, const char* name) {
	llvm::GlobalValue* orig = mangleMap[name];
	if(!orig)
		internalError(L"unable to find internal global value: %s", name);
	return (llvm::GlobalValue*)dest->getOrInsertGlobal(orig->getName(), orig->getType());
}

void VMKit::addSymbol(llvm::GlobalValue* gv) {
	const char* id = gv->getName().data();
	int   status;
	char* realname;
	realname = abi::__cxa_demangle(id, 0, 0, &status);
	const char* tmp = realname ? realname : id;
	uint32_t length = strlen(tmp);
	char* mangled = (char*)allocator()->allocate(length+1);
	strcpy(mangled, tmp);
	mangleMap[mangled] = gv;
	free(realname);
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

	for(llvm::Module::iterator cur=self()->begin(); cur!=self()->end(); cur++)
		addSymbol(cur);

	for(llvm::Module::global_iterator cur=self()->global_begin(); cur!=self()->global_end(); cur++)
		addSymbol(cur);

	_dataLayout = new llvm::DataLayout(self());

	llvm::GlobalValue* typeInfoGV = mangleMap["typeinfo for void*"];
	ptrTypeInfo = typeInfoGV ? dlsym(RTLD_SELF, typeInfoGV->getName().data()) : 0;

	if(!ptrTypeInfo)
		internalError(L"unable to find typeinfo for void*"); 
}


llvm::Function* VMKit::getGCRoot(llvm::Module* mod) {
	return llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::gcroot);
}

void VMKit::NotifyObjectEmitted(const llvm::ObjectImage &obj) {
	fprintf(stderr, "**** object jit event listener!\n");
}

void VMKit::NotifyFunctionEmitted(const llvm::Function &F,
																	void *Code,
																	size_t Size,
																	const llvm::JITEventListener::EmittedFunctionDetails &Details) {

	fprintf(stderr, "****  jit event listener!\n");
#if 0
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
#endif
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


ExceptionDescriptor::ExceptionDescriptor(const llvm::Function* llvmFunction, uintptr_t point, uintptr_t landingPad) {
	_llvmFunction = llvmFunction;
	_point = point;
	_landingPad = landingPad;
}
