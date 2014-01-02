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

void J3Method::markCompiled(llvm::Function* llvmFunction, void* fnPtr) {
	_llvmFunction = llvmFunction;
	_fnPtr = fnPtr;
}

void* J3Method::fnPtr(bool withCaller) {
	if(!isCompiled()) {
		//fprintf(stderr, "materializing: %s::%s%s\n", this, cl()->name()->cStr(), name()->cStr(), sign()->cStr());
		if(!isResolved()) {
			if(cl()->loader()->vm()->options()->debugLinking)
				fprintf(stderr, "linking %s::%s\n", cl()->name()->cStr(), name()->cStr());

			cl()->initialise();
			if(!isResolved())
				J3::noSuchMethodError("unable to find method", cl(), name(), sign());
		}

		J3CodeGen::translate(this, 1, withCaller);
 	}

	return _fnPtr;
}

void* J3Method::functionPointerOrStaticTrampoline() {
	if(isCompiled())
		return _fnPtr;
	if(!_staticTrampoline)
		_staticTrampoline = J3Trampoline::buildStaticTrampoline(cl()->loader()->allocator(), this);
	return _staticTrampoline;
}

void* J3Method::functionPointerOrVirtualTrampoline() {
	if(isCompiled())
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
		J3::internalError("trying to re-resolve a resolved method, should not happen");
	_index = index; 
}

void J3Method::postInitialise(uint32_t access, J3Attributes* attributes) {
	if((access & J3Cst::ACC_STATIC) != (_access & J3Cst::ACC_STATIC))
		J3::classFormatError(cl(), "trying to modify the static flag of %s", cl()->name()->cStr());
	_access = access;
	_attributes = attributes;
}

J3Method* J3Method::resolve(J3ObjectHandle* obj) {
	if(cl()->loader()->vm()->options()->debugLinking)
		fprintf(stderr, "virtual linking %s::%s\n", cl()->name()->cStr(), name()->cStr());
	vmkit::Names* n = cl()->loader()->vm()->names();
	return obj->vt()->type()->asObjectType()->findVirtualMethod(name(), sign());
}

J3Value J3Method::internalInvoke(bool statically, J3Value* inArgs) {
	J3Method* target = statically ? this : resolve(inArgs[0].valObject);

	void* fn = fnPtr(1);

	//fprintf(stderr, "Internal invoke %s::%s%s\n", target->cl()->name()->cStr(), target->name()->cStr(), target->sign()->cStr());

	J3LLVMSignature::function_t caller = methodType()->llvmSignature()->caller();
	if(!caller) {
		J3CodeGen::translate(this, 0, 1);
		caller = methodType()->llvmSignature()->caller();
	}
		
	J3Value res = caller(fn, inArgs);

	return res;
}

J3Value J3Method::internalInvoke(bool statically, J3ObjectHandle* handle, J3Value* inArgs) {
	J3Value* reIn;
	if(handle) {
		reIn = (J3Value*)alloca(methodType()->nbIns()*sizeof(J3Value));
		reIn[0].valObject = handle;
		memcpy(reIn+1, inArgs, (methodType()->nbIns() - 1)*sizeof(J3Value));
	} else
		reIn = inArgs;
	return internalInvoke(statically, reIn);
}

J3Value J3Method::internalInvoke(bool statically, J3ObjectHandle* handle, va_list va) {
	J3Value* args = (J3Value*)alloca(sizeof(J3Value)*methodType()->nbIns());
	J3* vm = cl()->loader()->vm();
	J3Type* cur;
	uint32_t i = 0;

	if(handle)
		args[i++].valObject = handle;

	for(; i<methodType()->nbIns(); i++) {
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

	return internalInvoke(statically, args);
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

void J3Method::dump() {
	printf("Method: %s %s::%s\n", sign()->cStr(), cl()->name()->cStr(), name()->cStr());
}

void J3Method::registerNative(void* fnPtr) {
	if(_nativeFnPtr)
		J3::noSuchMethodError("unable to dynamically modify a native function", cl(), name(), sign());
	_nativeFnPtr = fnPtr;
}

J3ObjectHandle* J3Method::javaMethod() {
	if(!_javaMethod) {
		cl()->lock();
		if(!_javaMethod) {
			J3ObjectHandle* prev = J3Thread::get()->tell();
			J3* vm = cl()->loader()->vm();

			if(name() == cl()->loader()->vm()->initName) {
				fprintf(stderr, " slot: %d\n", slot());
				_javaMethod = cl()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(vm->constructorClass));

				vm->constructorClassInit->invokeSpecial(_javaMethod,
																								cl()->javaClass(),
																								0, //	Class<?>[] parameterTypes,
																								0, // Class<?>[] checkedExceptions,
																								access(),
																								slot(),
																								vm->nameToString(sign()),
																								0, //byte[] annotations,
																								0); //byte[] parameterAnnotations)

				J3::internalError("implement me: java constructor");
			} else 
				J3::internalError("implement me: javaMethod");

			J3Thread::get()->restore(prev);
		}
		cl()->unlock();
	}

	return _javaMethod;
}


