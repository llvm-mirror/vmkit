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
#include "j3/j3reader.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Linker.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <vector>

using namespace j3;

J3Method::J3Method(uint16_t access, J3Class* cl, const vmkit::Name* name, J3Signature* signature) {
	_access = access;
	_cl = cl;
	_name = name;
	_signature = signature;
	_index = -1;
}

J3Attributes* J3Method::codeAttributes() { 
	if(!_codeAttributes) {
		J3* vm = J3Thread::get()->vm();
		J3Attribute* codeAttr = attributes()->lookup(vm->codeAttribute);
		if(codeAttr) {
			J3Reader reader(cl()->bytes());
			reader.seek(codeAttr->offset()+8, reader.SeekSet);
			reader.seek(reader.readU4(), reader.SeekCur);
			reader.seek(reader.readU2()*8, reader.SeekCur);
			_codeAttributes = cl()->readAttributes(&reader);
		}
	}
	return _codeAttributes;
}

vmkit::CompilationUnit* J3Method::unit() {
	return cl()->loader();
}

uint64_t J3Method::inlineWeight() {
	if(J3Thread::get()->vm()->options()->enableInlining)
		return vmkit::Symbol::inlineWeight();
	else
		return (uint64_t)-1;
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

void J3Method::aotSnapshot(llvm::Linker* linker) {
	if(_llvmFunction) {
		std::string err;
		linker->linkInModule(_llvmFunction->getParent(), llvm::Linker::DestroySource, &err);
	}
}

void J3Method::ensureCompiled(uint32_t mode) {
	if(((mode & J3CodeGen::WithMethod) && !fnPtr()) || ((mode & J3CodeGen::WithCaller) && !cxxCaller())) {
		//fprintf(stderr, "materializing: %s::%s%s\n", cl()->name()->cStr(), name()->cStr(), signature()->name()->cStr());
		J3CodeGen::translate(this, mode | cl()->loader()->compilationMode());
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

void* J3Method::getSymbolAddress() {
	return functionPointerOrStaticTrampoline();
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
	J3* vm = J3Thread::get()->vm();
	if(vm->options()->debugLinking)
		fprintf(stderr, "virtual linking %s::%s\n", cl()->name()->cStr(), name()->cStr());
	vmkit::Names* n = vm->names();
	return obj->vt()->type()->asObjectType()->findMethod(0, name(), signature());
}

J3Value J3Method::internalInvoke(J3Value* args) {
#if 1
	return cxxCaller()(fnPtr(), args);
#else
	J3Value res;
	try { 
		res = cxxCaller()(fnPtr(), args);
	} catch(void* e) {
		fprintf(stderr, " catch exception %p during the execution of %s::%s%s\n",
						e, cl()->name()->cStr(), name()->cStr(), signature()->name()->cStr());
		throw e;
	}
	return res;
#endif
}

J3Value J3Method::internalInvoke(J3ObjectHandle* handle, J3Value* inArgs) {
	cl()->initialise();
	ensureCompiled(J3CodeGen::WithMethod | J3CodeGen::WithCaller);  /* force the generation of the code and thus of the functionType */

	J3Value* reIn;
	if(handle) {
		uint32_t n = signature()->functionType(J3Cst::ACC_STATIC)->getNumParams();
		reIn = (J3Value*)alloca((n+1)*sizeof(J3Value));
		reIn[0].valObject = handle;
		memcpy(reIn+1, inArgs, n*sizeof(J3Value));
	} else
		reIn = inArgs;

	return internalInvoke(reIn);
}

J3Value J3Method::internalInvoke(J3ObjectHandle* handle, va_list va) {
	cl()->initialise();
	ensureCompiled(J3CodeGen::WithMethod | J3CodeGen::WithCaller);  /* force the generation of the code and thus of the functionType */

	llvm::FunctionType* fType = signature()->functionType(J3Cst::ACC_STATIC);      /* static signature for va */
	J3Value* args = (J3Value*)alloca(sizeof(J3Value)*(fType->getNumParams() + 1));
	J3* vm = J3Thread::get()->vm();
	uint32_t i = 0;

	if(handle)
		args[i++].valObject = handle;
	
	for(llvm::FunctionType::param_iterator cur=fType->param_begin(); cur!=fType->param_end(); cur++, i++) {
		llvm::Type* t = *cur;

		if(0) {}
#define doIt(id, ctype, lt, scale)							\
		else if(t == vm->type##id->llvmType())			\
			args[i].val##id = va_arg(va, ctype);
		onJavaPrimitives(doIt)
		else
			args[i].valObject = va_arg(va, J3ObjectHandle*);
#undef doIt
	}

	return internalInvoke(args);
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
}

char* J3Method::llvmFunctionName(J3Class* from) {
	if(!_llvmAllNames)
		buildLLVMNames(from ? from : cl());
	return _llvmAllNames + 5;
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
			J3* vm = J3Thread::get()->vm();

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

			if(name() == vm->initName) {
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
			} else {
				J3ObjectHandle* annotationDefault = cl()->asClass()->extractAttribute(attributes()->lookup(vm->annotationDefaultAttribute));
			
				_javaMethod = cl()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(vm->methodClass));

				vm->methodClassInit->invokeSpecial(_javaMethod,
																					 cl()->javaClass(0),
																					 vm->nameToString(name(), 0),
																					 parameters,
																					 signature()->javaOut()->javaClass(0),
																					 exceptions,
																					 access(),
																					 slot(),
																					 vm->nameToString(signature()->name(), 0),
																					 annotations,
																					 paramAnnotations,
																					 annotationDefault);
			}

			J3Thread::get()->restore(prev);
		}
		cl()->unlock();
	}

	return _javaMethod;
}


