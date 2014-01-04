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
#include "j3/j3attribute.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <vector>

using namespace j3;

J3Method::J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, J3Signature* signature) :
	_selfCode(this) {
	_access = access;
	_cl = cl;
	_name = name;
	_signature = signature;
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

void* J3Method::fnPtr() {
	return _fnPtr;
}

J3Signature::function_t J3Method::cxxCaller() {
	return signature()->caller(access());
}

void J3Method::ensureCompiled(bool withCaller) {
	if(!fnPtr() || (withCaller && !cxxCaller())) {
		// fprintf(stderr, "materializing: %s::%s%s\n", this, cl()->name()->cStr(), name()->cStr(), signature()->cStr());
		cl()->initialise();
		J3CodeGen::translate(this, !fnPtr(), withCaller);
 	}
}

void* J3Method::functionPointerOrStaticTrampoline() {
	if(fnPtr())
		return fnPtr();
	if(!_staticTrampoline)
		_staticTrampoline = J3Trampoline::buildStaticTrampoline(cl()->loader()->allocator(), this);
	return _staticTrampoline;
}

void* J3Method::functionPointerOrVirtualTrampoline() {
	if(fnPtr())
		return fnPtr();
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

void J3Method::setIndex(uint32_t index) { 
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
	return obj->vt()->type()->asObjectType()->findMethod(0, name(), signature());
}

J3Value J3Method::internalInvoke(J3ObjectHandle* handle, J3Value* inArgs) {
	ensureCompiled(1);  /* force the generation of the code and thus of the functionType */

	J3Value* reIn;
	if(handle) {
		uint32_t n = signature()->functionType(J3Cst::ACC_STATIC)->getNumParams();
		reIn = (J3Value*)alloca((n+1)*sizeof(J3Value));
		reIn[0].valObject = handle;
		memcpy(reIn+1, inArgs, n*sizeof(J3Value));
	} else
		reIn = inArgs;
	return cxxCaller()(fnPtr(), reIn);
}

J3Value J3Method::internalInvoke(J3ObjectHandle* handle, va_list va) {
	ensureCompiled(1);  /* force the generation of the code and thus of the functionType */

	llvm::FunctionType* fType = signature()->functionType(J3Cst::ACC_STATIC);      /* static signature for va */
	J3Value* args = (J3Value*)alloca(sizeof(J3Value)*(fType->getNumParams() + 1));
	J3* vm = cl()->loader()->vm();
	uint32_t i = 0;

	if(handle)
		args[i++].valObject = handle;
	
	for(llvm::FunctionType::param_iterator cur=fType->param_begin(); cur!=fType->param_end(); cur++, i++) {
		llvm::Type* t = *cur;

		if(t == vm->typeBoolean->llvmType())
			args[i].valBoolean = va_arg(va, bool);
		else if(t == vm->typeByte->llvmType())
			args[i].valByte = va_arg(va, int8_t);
		else if(t == vm->typeShort->llvmType())
			args[i].valShort = va_arg(va, int16_t);
		else if(t == vm->typeChar->llvmType())
			args[i].valChar = va_arg(va, uint16_t);
		else if(t == vm->typeInteger->llvmType())
			args[i].valInteger = va_arg(va, int32_t);
		else if(t == vm->typeLong->llvmType())
			args[i].valLong = va_arg(va, int64_t);
		else if(t == vm->typeFloat->llvmType())
			args[i].valFloat = va_arg(va, float);
		else if(t == vm->typeDouble->llvmType())
			args[i].valDouble = va_arg(va, double);
		else
			args[i].valObject = va_arg(va, J3ObjectHandle*);
	}

	return cxxCaller()(fnPtr(), args);
}

J3Value J3Method::invokeStatic(J3Value* args) {
	return internalInvoke(0, args);
}

J3Value J3Method::invokeStatic(va_list va) {
	return internalInvoke(0, va);
}

J3Value J3Method::invokeSpecial(J3ObjectHandle* handle, J3Value* args) {
	return internalInvoke(handle, args);
}

J3Value J3Method::invokeSpecial(J3ObjectHandle* handle, va_list va) {
	return internalInvoke(handle, va);
}

J3Value J3Method::invokeVirtual(J3ObjectHandle* handle, J3Value* args) {
	return resolve(handle)->internalInvoke(handle, args);
}

J3Value J3Method::invokeVirtual(J3ObjectHandle* handle, va_list va) {
	return resolve(handle)->internalInvoke(handle, va);
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

void J3Method::buildLLVMNames(J3Class* from) {
	const char* prefix = "stub_";
	uint32_t plen = 5;
	J3Mangler mangler(from);

	mangler.mangle(mangler.j3Id)->mangle(cl()->name(), name())->mangle(signature());

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
	printf("Method: %s %s::%s\n", signature()->name()->cStr(), cl()->name()->cStr(), name()->cStr());
}

void J3Method::registerNative(void* fnPtr) {
	if(_nativeFnPtr)
		J3::noSuchMethodError("unable to dynamically modify a native function", cl(), name(), signature());
	_nativeFnPtr = fnPtr;
}

J3Method* J3Method::nativeMethod(J3ObjectHandle* handle) {
	J3* vm = J3Thread::get()->vm();
	if(handle->vt()->type() == vm->constructorClass) {
		J3Class* cl = J3ObjectType::nativeClass(handle->getObject(vm->constructorClassClass))->asClass();
		uint32_t slot = handle->getInteger(vm->constructorClassSlot);
		J3Method* res = cl->methods()[slot];
		if(res->name() != vm->initName)
			J3::internalError("nativeName with a java.lang.reflect.Constructor with a non-constructot method");
		return res;
	} else
		J3::internalError("implement me: nativeMethod with method");
}

J3ObjectHandle* J3Method::javaMethod() {
	if(!_javaMethod) {
		cl()->lock();
		if(!_javaMethod) {
			J3ObjectHandle* prev = J3Thread::get()->tell();
			J3* vm = cl()->loader()->vm();

			uint32_t nbIns = signature()->nbIns();

			J3ObjectHandle* parameters = J3ObjectHandle::doNewArray(vm->classClass->getArray(), nbIns);

			for(uint32_t i=0; i<nbIns; i++)
				parameters->setObjectAt(i, signature()->javaIns(i)->javaClass(0));

			J3Attribute* exceptionAttribute = attributes()->lookup(vm->exceptionsAttribute);
			J3ObjectHandle* exceptions;

			if(exceptionAttribute)
				J3::internalError("implement me: excp");
			else
				exceptions = J3ObjectHandle::doNewArray(vm->classClass->getArray(), 0);

			J3ObjectHandle* annotations = cl()->asClass()->extractAttribute(attributes()->lookup(vm->annotationsAttribute));
			J3ObjectHandle* paramAnnotations = cl()->asClass()->extractAttribute(attributes()->lookup(vm->paramAnnotationsAttribute));

			if(name() == cl()->loader()->vm()->initName) {
				_javaMethod = cl()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(vm->constructorClass));

				vm->constructorClassInit->invokeSpecial(_javaMethod,
																								cl()->javaClass(0),
																								parameters,
																								exceptions,
																								access(),
																								slot(),
																								vm->nameToString(signature()->name(), 0),
																								annotations,
																								paramAnnotations);
			} else 
				J3::internalError("implement me: javaMethod");

			J3Thread::get()->restore(prev);
		}
		cl()->unlock();
	}

	return _javaMethod;
}


