#include "vmkit/safepoint.h"

#include "j3/j3method.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3constants.h"
#include "j3/j3.h"
#include "j3/j3mangler.h"
#include "j3/j3thread.h"
#include "j3/j3codegen.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <vector>

using namespace j3;

J3MethodType::J3MethodType(J3Type** args, size_t nbArgs) {
	_out = args[nbArgs-1];
	_nbIns = nbArgs-1;
	memcpy(_ins, args, (nbArgs-1)*sizeof(J3Type*));
			
	std::vector<llvm::Type*> in;
	for(uint32_t i=0; i<nbIns(); i++)
		in.push_back(ins(i)->llvmType());
	_llvmType = llvm::FunctionType::get(out()->llvmType(), in, 0);
}

J3Method::J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, const vmkit::Name* sign) :
	_selfCode(this) {
	_access = access;
	_cl = cl;
	_name = name;
	_sign = sign;
	_index = -1;
}

uint32_t J3Method::index()  { 
	return _index; 
}

uint8_t* J3Method::fnPtr() {
	if(!_fnPtr) {
		//fprintf(stderr, "materializing: %ls::%ls%ls\n", cl()->name()->cStr(), name()->cStr(), sign()->cStr());
		if(!isResolved()) {
			if(cl()->loader()->vm()->options()->debugLinking)
				fprintf(stderr, "linking %ls::%ls\n", cl()->name()->cStr(), name()->cStr());

			cl()->initialise();
			if(!isResolved())
				J3::noSuchMethodError(L"unable to find method", cl(), name(), sign());
		}

		llvm::Module* module = new llvm::Module(llvmFunctionName(), cl()->loader()->vm()->llvmContext());
		_llvmFunction = llvmFunction(0, module);

		J3CodeGen::translate(this, _llvmFunction);

		llvm::ExecutionEngine* ee = cl()->loader()->ee();
		cl()->loader()->addModule(module);
		_fnPtr = (uint8_t*)ee->getFunctionAddress(_llvmFunction->getName().data());

#if 1
		fprintf(stderr, "%s is generated at %p\n", llvmFunctionName(), _fnPtr);
		llvm::SmallString<256> symName;
		symName += module->getModuleIdentifier();
		symName += "__frametable";
		vmkit::Safepoint* sf = (vmkit::Safepoint*)ee->getGlobalValueAddress(symName.c_str());

		if(!sf)
			cl()->loader()->vm()->internalError(L"unable to find safepoints");
		
		while(sf->addr()) {
			fprintf(stderr, "  [%p] safepoint at %p for function %p::%d\n", sf, sf->addr(), sf->metaData(), sf->sourceIndex());
			for(uint32_t i=0; i<sf->nbLives(); i++)
				fprintf(stderr, "    live at %d\n", sf->liveAt(i));

			sf = sf->getNext();
		}
#endif
	}

	return _fnPtr;
}

uint8_t* J3Method::functionPointerOrTrampoline() {
	return _fnPtr ? _fnPtr : _trampoline;
}

uint8_t* J3MethodCode::getSymbolAddress() {
	return self->functionPointerOrTrampoline();
}

uint8_t* J3Method::getSymbolAddress() {
	return (uint8_t*)this;
}

void J3Method::setResolved(uint32_t index) { 
	if(isResolved())
		J3::internalError(L"trying to re-resolve a resolved method, should not happen");
	_index = index; 
}

void J3Method::postInitialise(uint32_t access, J3Attributes* attributes) {
	if((access & J3Cst::ACC_STATIC) != (_access & J3Cst::ACC_STATIC))
		J3::classFormatError(cl(), L"trying to modify the static flag of %ls", cl()->name()->cStr());
	_access = access;
	_attributes = attributes;
}

J3Method* J3Method::resolve(J3ObjectHandle* obj) {
	if(cl()->loader()->vm()->options()->debugLinking)
		fprintf(stderr, "virtual linking %ls::%ls\n", cl()->name()->cStr(), name()->cStr());
	vmkit::Names* n = cl()->loader()->vm()->names();
	return cl()->findVirtualMethod(name(), sign());
}

void* J3Method::operator new(size_t size, vmkit::BumpAllocator* allocator, size_t trampolineSize) {
	return allocator->allocate(size + (trampolineSize - 1)*sizeof(uint8_t));
}

J3Method* J3Method::newMethod(vmkit::BumpAllocator* allocator, 
															uint16_t access, 
															J3Class* cl, 
															const vmkit::Name* name, 
															const vmkit::Name* sign) {
	size_t trampolineSize = 148;
	
	void* tra = (void*)J3ObjectHandle::trampoline;
	J3Method* res = new(allocator, trampolineSize) J3Method(access, cl, name, sign);

#define dd(p, n) ((((uintptr_t)p) >> n) & 0xff)
	uint8_t code[] = {
		0x57, // 0: push %rdi
		0x56, // 1: push %rsi
		0x52, // 2: push %rdx
		0x51, // 3: push %rcx
		0x41, 0x50, // 4: push %r8
		0x41, 0x51, // 6: push %r9
		0x48, 0x81, 0xec, 0x88, 0x00, 0x00, 0x00, // 8: sub $128+8, %esp
		0xf3, 0x0f, 0x11, 0x04, 0x24,             // 15: movss %xmm0, (%rsp)
		0xf3, 0x0f, 0x11, 0x4c, 0x24, 0x10,       // 20: movss %xmm1, 16(%rsp)
		0xf3, 0x0f, 0x11, 0x54, 0x24, 0x20,       // 26: movss %xmm2, 32(%rsp)
		0xf3, 0x0f, 0x11, 0x5c, 0x24, 0x30,       // 32: movss %xmm3, 48(%rsp)
		0xf3, 0x0f, 0x11, 0x64, 0x24, 0x40,       // 38: movss %xmm4, 64(%rsp)
		0xf3, 0x0f, 0x11, 0x6c, 0x24, 0x50,       // 44: movss %xmm5, 80(%rsp)
		0xf3, 0x0f, 0x11, 0x74, 0x24, 0x60,       // 50: movss %xmm6, 96(%rsp)
		0xf3, 0x0f, 0x11, 0x7c, 0x24, 0x70,       // 56: movss %xmm7, 112(%rsp)
		0x48, 0xbe, dd(res, 0), dd(res, 8), dd(res, 16), dd(res, 24), dd(res, 32), dd(res, 40), dd(res, 48), dd(res, 56), // 62: mov -> %rsi
		0x48, 0xb8, dd(tra, 0), dd(tra, 8), dd(tra, 16), dd(tra, 24), dd(tra, 32), dd(tra, 40), dd(tra, 48), dd(tra, 56), // 72: mov -> %rax
		0xff, 0xd0, // 82: call %rax
		0xf3, 0x0f, 0x10, 0x04, 0x24,             // 84: movss (%rsp), %xmm0
		0xf3, 0x0f, 0x10, 0x4c, 0x24, 0x10,       // 89: movss 16(%rsp), %xmm1
		0xf3, 0x0f, 0x10, 0x54, 0x24, 0x20,       // 95: movss 32(%rsp), %xmm2
		0xf3, 0x0f, 0x10, 0x5c, 0x24, 0x30,       // 101: movss 48(%rsp), %xmm3
		0xf3, 0x0f, 0x10, 0x64, 0x24, 0x40,       // 107: movss 64(%rsp), %xmm4
		0xf3, 0x0f, 0x10, 0x6c, 0x24, 0x50,       // 113: movss 80(%rsp), %xmm5
		0xf3, 0x0f, 0x10, 0x74, 0x24, 0x60,       // 119: movss 96(%rsp), %xmm6
		0xf3, 0x0f, 0x10, 0x7c, 0x24, 0x70,       // 125: movss 112(%rsp), %xmm7
		0x48, 0x81, 0xc4, 0x88, 0x00, 0x00, 0x00, // 131: add $128+8, %esp
		0x41, 0x59, // 138: pop %r9
		0x41, 0x58, // 140: pop %r8
		0x59, // 142: pop %rcx
		0x5a, // 143: pop %rdx
		0x5e, // 144: pop %rsi
		0x5f, // 145: pop %rdi
		0xff, 0xe0 // 146: jmp %rax
		// total: 148
	};
#undef dd

	memcpy(res->_trampoline, code, trampolineSize);

	return res;
}


J3Value J3Method::internalInvoke(bool statically, J3ObjectHandle* handle, J3Value* inArgs) {
	std::vector<llvm::GenericValue> args(methodType()->nbIns());
	J3* vm = cl()->loader()->vm();
	J3Type* cur;
	uint32_t i = 0;

	if(handle)
		args[i++].PointerVal = handle->obj();

	for(; i<methodType()->nbIns(); i++) {  /* have to avoid collection at this point */
		cur = methodType()->ins(i);
		
		if(cur == vm->typeBoolean)
			args[i].IntVal = inArgs[i].valBoolean;
		else if(cur == vm->typeByte)
			args[i].IntVal = inArgs[i].valByte;
		else if(cur == vm->typeShort)
			args[i].IntVal = inArgs[i].valShort;
		else if(cur == vm->typeChar)
			args[i].IntVal = inArgs[i].valChar;
		else if(cur == vm->typeInteger)
			args[i].IntVal = inArgs[i].valInteger;
		else if(cur == vm->typeLong)
			args[i].IntVal = inArgs[i].valLong;
		else if(cur == vm->typeFloat)
			args[i].FloatVal = inArgs[i].valFloat;
		else if(cur == vm->typeDouble)
			args[i].FloatVal = inArgs[i].valDouble;
		else
			args[i].PointerVal = inArgs[i].valObject->obj();
	}

	J3Method* target;

	if(statically)
		target = this;
	else {
		/* can not use trampoline here */
		J3ObjectHandle* self = (J3ObjectHandle*)args[0].PointerVal;
		target = resolve(self);
	}

	target->fnPtr(); /* ensure that the function is compiled */
	cl()->loader()->oldee()->updateGlobalMapping(target->_llvmFunction, target->fnPtr());
	llvm::GenericValue res = cl()->loader()->oldee()->runFunction(target->_llvmFunction, args);

	J3Value holder;
	cur = methodType()->out();
	
	if(cur == vm->typeBoolean)
		holder.valBoolean = (bool)res.IntVal.getZExtValue();
	else if(cur == vm->typeByte)
		holder.valByte = (int8_t)res.IntVal.getZExtValue();
	else if(cur == vm->typeShort)
		holder.valShort = (int16_t)res.IntVal.getZExtValue();
	else if(cur == vm->typeChar)
		holder.valChar = (uint16_t)res.IntVal.getZExtValue();
	else if(cur == vm->typeInteger)
		holder.valInteger = (int32_t)res.IntVal.getZExtValue();
	else if(cur == vm->typeLong)
		holder.valLong = res.IntVal.getZExtValue();
	else if(cur == vm->typeFloat)
		holder.valFloat = res.FloatVal;
	else if(cur == vm->typeDouble)
		holder.valDouble = res.FloatVal;
	else if(cur != vm->typeVoid)
		holder.valObject = J3Thread::get()->push((J3Object*)res.PointerVal);

	return holder;
}

J3Value J3Method::internalInvoke(bool statically, J3ObjectHandle* handle, va_list va) {
	J3Value* args = (J3Value*)alloca(sizeof(J3Value)*methodType()->nbIns());
	J3* vm = cl()->loader()->vm();
	J3Type* cur;
	uint32_t i = 0;

	if(handle)
		args[i++].valObject = handle;

	for(; i<methodType()->nbIns(); i++) {  /* have to avoid collection at this point */
		cur = methodType()->ins(i);

		if(cur == vm->typeBoolean)
			args[i].valBoolean = va_arg(va, bool);
		else if(cur == vm->typeByte)
			args[i].valByte = va_arg(va, int8_t);
		else if(cur == vm->typeShort)
			args[i].valShort = va_arg(va, int16_t);
		else if(cur == vm->typeChar)
			args[i].valChar = va_arg(va, uint16_t);
		else if(cur == vm->typeInteger)
			args[i].valInteger = va_arg(va, int32_t);
		else if(cur == vm->typeLong)
			args[i].valLong = va_arg(va, int64_t);
		else if(cur == vm->typeFloat)
			args[i].valFloat = va_arg(va, float);
		else if(cur == vm->typeDouble)
			args[i].valDouble = va_arg(va, double);
		else
			args[i].valObject = va_arg(va, J3ObjectHandle*);
	}

	return internalInvoke(statically, 0, args);
}

J3Value J3Method::invokeStatic(J3Value* args) {
	return internalInvoke(1, 0, args);
}

J3Value J3Method::invokeSpecial(J3ObjectHandle* handle, J3Value* args) {
	return internalInvoke(1, handle, args);
}

J3Value J3Method::invokeVirtual(J3ObjectHandle* handle, J3Value* args) {
	return internalInvoke(0, handle, args);
}

J3Value J3Method::invokeStatic(va_list va) {
	return internalInvoke(1, 0, va);
}

J3Value J3Method::invokeSpecial(J3ObjectHandle* handle, va_list va) {
	return internalInvoke(1, handle, va);
}

J3Value J3Method::invokeVirtual(J3ObjectHandle* handle, va_list va) {
	return internalInvoke(0, handle, va);
}

J3Value J3Method::invokeStatic(...) {
	va_list va;
	va_start(va, this);
	J3Value res = invokeStatic(va);
	va_end(va);
	return res;
}

J3Value J3Method::invokeSpecial(J3ObjectHandle* handle, ...) {
	va_list va;
	va_start(va, this);
	J3Value res = invokeSpecial(handle, va);
	va_end(va);
	return res;
}

J3Value J3Method::invokeVirtual(J3ObjectHandle* handle, ...) {
	va_list va;
	va_start(va, this);
	J3Value res = invokeVirtual(handle, va);
	va_end(va);
	return res;
}

J3MethodType* J3Method::methodType(J3Class* from) {
	if(!_methodType) {
		const vmkit::Name* realSign = sign();
		J3ClassLoader*     loader = cl()->loader();

		if(!J3Cst::isStatic(access()))
			realSign = J3Cst::rewrite(loader->vm(), cl()->name(), sign());

		_methodType = loader->getMethodType(from ? from : cl(), realSign);
	}

	return _methodType;
}

void J3Method::buildLLVMNames(J3Class* from) {
	const char* prefix = "stub_";
	uint32_t plen = 5;
	J3Mangler mangler(from);

	mangler.mangle(mangler.j3Id)->mangle(this)->mangleType(this);

	uint32_t length = mangler.length() + plen;
	_llvmAllNames = (char*)cl()->loader()->allocator()->allocate(length + 1);
	memcpy(_llvmAllNames, prefix, plen);
	memcpy(_llvmAllNames+plen, mangler.cStr(), mangler.length());
	_llvmAllNames[length] = 0;

	cl()->loader()->addSymbol(_llvmAllNames+0,   &_selfCode);
	cl()->loader()->addSymbol(_llvmAllNames+4, this);
	cl()->loader()->addSymbol(_llvmAllNames+plen, &_selfCode);
}

char* J3Method::llvmFunctionName(J3Class* from) {
	if(!_llvmAllNames)
		buildLLVMNames(from ? from : cl());
	return _llvmAllNames + 5;
}

char* J3Method::llvmDescriptorName(J3Class* from) {
	if(!_llvmAllNames)
		buildLLVMNames(from ? from : cl());
	return _llvmAllNames + 4;
}

char* J3Method::llvmStubName(J3Class* from) {
	if(!_llvmAllNames)
		buildLLVMNames(from ? from : cl());
	return _llvmAllNames + 0;
}

llvm::GlobalValue* J3Method::llvmDescriptor(llvm::Module* module) {
	return llvm::cast<llvm::GlobalValue>(module->getOrInsertGlobal(llvmDescriptorName(), cl()->loader()->vm()->typeJ3Method));
}

llvm::Function* J3Method::llvmFunction(bool isStub, llvm::Module* module, J3Class* from) {
	const char* id = (isStub && !_fnPtr) ? llvmStubName(from) : llvmFunctionName(from);
	return (llvm::Function*)module->getOrInsertFunction(id, methodType(from ? from : cl())->llvmType());
}

void J3Method::dump() {
	printf("Method: %ls %ls::%ls\n", sign()->cStr(), cl()->name()->cStr(), name()->cStr());
}

void J3Method::registerNative(void* fnPtr) {
	if(_nativeFnPtr)
		J3::noSuchMethodError(L"unable to dynamically modify a native function", cl(), name(), sign());
	_nativeFnPtr = fnPtr;
}

llvm::Type* J3Method::doNativeType(J3Type* type) {
	llvm::Type* t = type->llvmType();

	if(t->isPointerTy())
		return cl()->loader()->vm()->typeJ3ObjectHandlePtr;
	else
		return t;
}

llvm::Function* J3Method::nativeLLVMFunction(llvm::Module* module) {
	J3ClassLoader* loader = cl()->loader();
	J3Mangler      mangler(cl());

	mangler.mangle(mangler.javaId)->mangle(this);
	uint32_t length = mangler.length();
	mangler.mangleType(this);

	void* fnPtr = _nativeFnPtr;

	if(!fnPtr)
		fnPtr = loader->lookupNativeFunctionPointer(this, mangler.cStr());

	if(!fnPtr) {
		mangler.cStr()[length] = 0;
		fnPtr = loader->lookupNativeFunctionPointer(this, mangler.mangleType(this)->cStr());
	}

	if(!fnPtr)
		J3::linkageError(this);

	J3MethodType*            type = methodType();
	std::vector<llvm::Type*> nativeIns;
	llvm::Type*              nativeOut;

	nativeIns.push_back(loader->vm()->typeJNIEnvPtr);

	if(J3Cst::isStatic(access()))
		nativeIns.push_back(doNativeType(loader->vm()->classClass));
			
	for(int i=0; i<type->nbIns(); i++)
		nativeIns.push_back(doNativeType(type->ins(i)));

	nativeOut = doNativeType(type->out());

	char* buf = (char*)cl()->loader()->allocator()->allocate(mangler.length()+1);
	memcpy(buf, mangler.cStr(), mangler.length()+1);

	llvm::FunctionType* fType = llvm::FunctionType::get(nativeOut, nativeIns, 0);
	llvm::Function* res = llvm::Function::Create(fType,
																							 llvm::GlobalValue::ExternalLinkage,
																							 buf,
																							 module);

	cl()->loader()->addSymbol(buf, new(cl()->loader()->allocator()) vmkit::NativeSymbol((uint8_t*)fnPtr));

	return res;
}
