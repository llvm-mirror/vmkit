#include "vmkit/safepoint.h"

#include "j3/j3method.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3constants.h"
#include "j3/j3.h"
#include "j3/j3mangler.h"
#include "j3/j3thread.h"
#include "j3/j3codegen.h"
#include "j3/j3trampoline.h"

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
			
}

llvm::FunctionType* J3MethodType::unsafe_llvmFunctionType() {
	if(!_llvmFunctionType) {
		std::vector<llvm::Type*> in;
		for(uint32_t i=0; i<nbIns(); i++)
			in.push_back(ins(i)->llvmType());
		_llvmFunctionType = llvm::FunctionType::get(out()->llvmType(), in, 0);
	}
	return _llvmFunctionType;
}

J3Method::J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, const vmkit::Name* sign) :
	_selfCode(this) {
	_access = access;
	_cl = cl;
	_name = name;
	_sign = sign;
	_index = -1;
}

uint32_t J3Method::interfaceIndex() {
	return cl()->loader()->interfaceIndex(this);
}

uint32_t J3Method::index()  { 
	return _index; 
}

void* J3Method::fnPtr() {
	if(!_fnPtr) {
		//fprintf(stderr, "materializing: %ls::%ls%ls\n", this, cl()->name()->cStr(), name()->cStr(), sign()->cStr());
		if(!isResolved()) {
			if(cl()->loader()->vm()->options()->debugLinking)
				fprintf(stderr, "linking %ls::%ls\n", cl()->name()->cStr(), name()->cStr());

			cl()->initialise();
			if(!isResolved())
				J3::noSuchMethodError(L"unable to find method", cl(), name(), sign());
		}

		cl()->loader()->vm()->lockCompiler();
		llvm::Module* module = new llvm::Module(llvmFunctionName(), cl()->loader()->vm()->llvmContext());
		_llvmFunction = unsafe_llvmFunction(0, module);

		J3CodeGen::translate(this, _llvmFunction);

		cl()->loader()->compileModule(module);

		_fnPtr = (void*)cl()->loader()->ee()->getFunctionAddress(_llvmFunction->getName().data());
		cl()->loader()->vm()->unlockCompiler();
 	}

	return _fnPtr;
}

void* J3Method::functionPointerOrStaticTrampoline() {
	if(_fnPtr)
		return _fnPtr;
	if(!_staticTrampoline)
		_staticTrampoline = J3Trampoline::buildStaticTrampoline(cl()->loader()->allocator(), this);
	return _staticTrampoline;
}

void* J3Method::functionPointerOrVirtualTrampoline() {
	if(_fnPtr)
		return _fnPtr;
	if(!_virtualTrampoline)
		_virtualTrampoline = J3Trampoline::buildVirtualTrampoline(cl()->loader()->allocator(), this);
	return _virtualTrampoline;
}

void* J3MethodCode::getSymbolAddress() {
	return self->functionPointerOrStaticTrampoline();
}

void* J3Method::getSymbolAddress() {
	return this;
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
	return obj->vt()->type()->asObjectType()->findVirtualMethod(name(), sign());
}


J3Value J3Method::internalInvoke(bool statically, J3ObjectHandle* handle, J3Value* inArgs) {
	std::vector<llvm::GenericValue> args(methodType()->nbIns());
	J3* vm = cl()->loader()->vm();
	J3Type* cur;
	uint32_t i = 0, d = 0;

	if(handle) {
		args[i++].PointerVal = handle->obj();
		d = 1;
	}

	for(; i<methodType()->nbIns(); i++) {  /* have to avoid collection at this point */
		cur = methodType()->ins(i);
		
		if(cur == vm->typeBoolean)
			args[i].IntVal = inArgs[i-d].valBoolean;
		else if(cur == vm->typeByte)
			args[i].IntVal = inArgs[i-d].valByte;
		else if(cur == vm->typeShort)
			args[i].IntVal = inArgs[i-d].valShort;
		else if(cur == vm->typeChar)
			args[i].IntVal = inArgs[i-d].valChar;
		else if(cur == vm->typeInteger)
			args[i].IntVal = inArgs[i-d].valInteger;
		else if(cur == vm->typeLong)
			args[i].IntVal = inArgs[i-d].valLong;
		else if(cur == vm->typeFloat)
			args[i].FloatVal = inArgs[i-d].valFloat;
		else if(cur == vm->typeDouble)
			args[i].FloatVal = inArgs[i-d].valDouble;
		else
			args[i].PointerVal = inArgs[i-d].valObject->obj();
	}

	J3Method* target;

	if(statically)
		target = this;
	else {
		J3ObjectHandle* self = handle ? handle : inArgs[0].valObject;
		target = resolve(self);
	}

	//fprintf(stderr, "invoke: %ls::%ls%ls\n", target->cl()->name()->cStr(), target->name()->cStr(), target->sign()->cStr());
	target->fnPtr(); /* ensure that the function is compiled */
	cl()->loader()->vm()->lockCompiler();
	cl()->loader()->oldee()->updateGlobalMapping(target->_llvmFunction, target->fnPtr());
	cl()->loader()->vm()->unlockCompiler();
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
	uint32_t i = 0, d = 0;

	if(handle)
		i = d = 1;

	for(; i<methodType()->nbIns(); i++) {
		cur = methodType()->ins(i);

		if(cur == vm->typeBoolean)
			args[i-d].valBoolean = va_arg(va, bool);
		else if(cur == vm->typeByte)
			args[i-d].valByte = va_arg(va, int8_t);
		else if(cur == vm->typeShort)
			args[i-d].valShort = va_arg(va, int16_t);
		else if(cur == vm->typeChar)
			args[i-d].valChar = va_arg(va, uint16_t);
		else if(cur == vm->typeInteger)
			args[i-d].valInteger = va_arg(va, int32_t);
		else if(cur == vm->typeLong)
			args[i-d].valLong = va_arg(va, int64_t);
		else if(cur == vm->typeFloat)
			args[i-d].valFloat = va_arg(va, float);
		else if(cur == vm->typeDouble)
			args[i-d].valDouble = va_arg(va, double);
		else
			args[i-d].valObject = va_arg(va, J3ObjectHandle*);
	}

	return internalInvoke(statically, handle, args);
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

llvm::GlobalValue* J3Method::unsafe_llvmDescriptor(llvm::Module* module) {
	return llvm::cast<llvm::GlobalValue>(module->getOrInsertGlobal(llvmDescriptorName(), cl()->loader()->vm()->typeJ3Method));
}

llvm::Function* J3Method::unsafe_llvmFunction(bool isStub, llvm::Module* module, J3Class* from) {
	const char* id = (isStub && !_fnPtr) ? llvmStubName(from) : llvmFunctionName(from);
	return (llvm::Function*)module->getOrInsertFunction(id, methodType(from ? from : cl())->unsafe_llvmFunctionType());
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

llvm::Function* J3Method::unsafe_nativeLLVMFunction(llvm::Module* module) {
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

	cl()->loader()->addSymbol(buf, new(cl()->loader()->allocator()) vmkit::NativeSymbol(fnPtr));

	return res;
}
