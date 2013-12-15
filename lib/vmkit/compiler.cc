#include <dlfcn.h>

#include "vmkit/compiler.h"
#include "vmkit/thread.h"
#include "vmkit/vmkit.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Module.h"

using namespace vmkit;

uint8_t* Symbol::getSymbolAddress() {
	Thread::get()->vm()->internalError(L"implement me: getSymbolAddress");
}

void* CompilationUnit::operator new(size_t n, BumpAllocator* allocator) {
	return allocator->allocate(n);
}

void  CompilationUnit::operator delete(void* self) {
}

CompilationUnit::CompilationUnit(BumpAllocator* allocator, const char* id) :
	_symbolTable(vmkit::Util::char_less, allocator) {
	_allocator = allocator;
	pthread_mutex_init(&_mutexSymbolTable, 0);

	std::string err;
	_ee = llvm::EngineBuilder(new llvm::Module(id, Thread::get()->vm()->llvmContext()))
		.setUseMCJIT(1)
		.setMCJITMemoryManager(this)
		.setErrorStr(&err)
		.create();

	if (!ee())
		Thread::get()->vm()->internalError(L"Error while creating execution engine: %s\n", err.c_str());

  ee()->RegisterJITEventListener(Thread::get()->vm());

	_oldee = llvm::EngineBuilder(new llvm::Module("old ee", Thread::get()->vm()->llvmContext()))
		.setErrorStr(&err)
		.create();

	if (!oldee())
		Thread::get()->vm()->internalError(L"Error while creating execution engine: %s\n", err.c_str());

	oldee()->DisableLazyCompilation(0);
}

CompilationUnit::~CompilationUnit() {
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

uint64_t CompilationUnit::getSymbolAddress(const std::string &Name) {
	pthread_mutex_lock(&_mutexSymbolTable);
	const char* id = Name.c_str() + 1;

	std::map<const char*, vmkit::Symbol*>::iterator it = _symbolTable.find(id);
	vmkit::Symbol* res;

	if(it == _symbolTable.end()) {
		uint8_t* addr = (uint8_t*)dlsym(RTLD_SELF, id);
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
	return (uint64_t)(uintptr_t)res->getSymbolAddress();
}
