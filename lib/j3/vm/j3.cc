#include <stdio.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "j3/j3class.h"
#include "j3/j3.h"
#include "j3/j3classloader.h"
#include "j3/j3constants.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3trampoline.h"
#include "j3/j3lib.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

#include "vmkit/safepoint.h"
#include "vmkit/system.h"

using namespace j3;

vmkit::T_ptr_less_t<J3ObjectHandle*> J3::charArrayLess;

J3::J3(vmkit::BumpAllocator* allocator) : 
	VMKit(allocator),
	nameToCharArrays(vmkit::Name::less, allocator),
	charArrayToStrings(charArrayLess, allocator),
	_names(allocator) {
	pthread_mutex_init(&stringsMutex, 0);
	constantValueAttr = names()->get(J3Cst::constantValueAttr);
	codeAttr =          names()->get(J3Cst::codeAttr);
	clinitName =        names()->get(J3Cst::clinitName);
	clinitSign =        names()->get(J3Cst::clinitSign);
	initName =          names()->get(J3Cst::initName);
	interfaceTrampoline = J3Trampoline::buildInterfaceTrampoline(allocator);
}

J3* J3::create() {
	vmkit::BumpAllocator* allocator = vmkit::BumpAllocator::create();
	return new(allocator) J3(allocator);
}

void J3::introspect() {
	typeJNIEnvPtr           = llvm::PointerType::getUnqual(introspectType("struct.JNIEnv_"));
	typeJ3VirtualTablePtr   = llvm::PointerType::getUnqual(introspectType("class.j3::J3VirtualTable"));
	typeJ3Type              = introspectType("class.j3::J3Type");
	typeJ3TypePtr           = llvm::PointerType::getUnqual(typeJ3Type);
	typeJ3ObjectTypePtr     = llvm::PointerType::getUnqual(introspectType("class.j3::J3ObjectType"));
	typeJ3Class             = introspectType("class.j3::J3Class");
	typeJ3ClassPtr          = llvm::PointerType::getUnqual(typeJ3Class);
	typeJ3ArrayClass        = introspectType("class.j3::J3ArrayClass");
	typeJ3ArrayClassPtr     = llvm::PointerType::getUnqual(typeJ3ArrayClass);
	typeJ3ArrayObject       = introspectType("class.j3::J3ArrayObject");
	typeJ3ArrayObjectPtr    = llvm::PointerType::getUnqual(typeJ3ArrayObject);
	typeJ3Method            = introspectType("class.j3::J3Method");
	typeJ3Object            = introspectType("class.j3::J3Object");
	typeJ3ObjectPtr         = llvm::PointerType::getUnqual(typeJ3Object);
	typeJ3ObjectHandlePtr   = llvm::PointerType::getUnqual(introspectType("class.j3::J3ObjectHandle"));

	typeGXXException        = llvm::StructType::get(llvm::Type::getInt8Ty(llvmContext())->getPointerTo(), 
																									llvm::Type::getInt32Ty(llvmContext()), NULL);
}

void J3::start(int argc, char** argv) {
	_options.process(argc, argv);

	vmkit::ThreadAllocator::initialize(sizeof(J3Thread), options()->stackSize);

	J3Thread* thread = new J3ThreadBootstrap(this);

	vmkitBootstrap(thread, options()->selfBitCodePath);	
}

void J3::run() {
	introspect();

	vmkit::BumpAllocator* loaderAllocator = vmkit::BumpAllocator::create();
	initialClassLoader = 
		new(loaderAllocator) 
		J3InitialClassLoader(this, options()->rtJar, loaderAllocator);

	vmkit::BumpAllocator* a = initialClassLoader->allocator();

#define defPrimitive(name, ctype, llvmtype)			\
	type##name = new(a) J3Primitive(initialClassLoader, J3Cst::ID_##name, llvm::Type::get##llvmtype##Ty(llvmContext()));
	onJavaTypes(defPrimitive)
#undef defPrimitive

	nbArrayInterfaces    = 2;
	arrayInterfaces      = (J3Type**)initialClassLoader->allocator()->allocate(2*sizeof(J3Type*));
	arrayInterfaces[0]   = initialClassLoader->getClass(names()->get(L"java/lang/Cloneable"));
	arrayInterfaces[1]   = initialClassLoader->getClass(names()->get(L"java/io/Serializable"));

	charArrayClass           = typeChar->getArray();
	objectClass              = initialClassLoader->getClass(names()->get(L"java/lang/Object"));
	
	stringClass              = initialClassLoader->getClass(names()->get(L"java/lang/String"));
	stringInit               = initialClassLoader->method(0, stringClass, initName, names()->get(L"([CZ)V"));
	stringValue              = stringClass->findVirtualField(names()->get(L"value"), typeChar->getArray());

	classClass               = initialClassLoader->getClass(names()->get(L"java/lang/Class"));
	J3Field hf(J3Cst::ACC_PRIVATE, names()->get(L"** vmData **"), typeLong);
	classClass->resolve(&hf, 1);
	classInit                = initialClassLoader->method(0, classClass, initName, names()->get(L"()V"));
	classVMData              = classClass->findVirtualField(hf.name(), hf.type());

	threadVMData             = initialClassLoader->getClass(names()->get("java/lang/Thread"))
		->findVirtualField(names()->get(L"eetop"), typeLong);
	threadRun                = initialClassLoader->method(0, L"java/lang/Thread", L"run", L"()V");

	J3Lib::bootstrap(this);
}

JNIEnv* J3::jniEnv() {
	return J3Thread::get()->jniEnv();
}

J3ObjectHandle* J3::arrayToString(J3ObjectHandle* array) {
	pthread_mutex_lock(&stringsMutex);
	J3ObjectHandle* res = charArrayToStrings[array];
	if(!res) {
		J3ObjectHandle* prev = J3Thread::get()->tell();
		res = initialClassLoader->globalReferences()->add(J3ObjectHandle::doNewObject(stringClass));
		J3Thread::get()->restore(prev);

		stringInit->invokeSpecial(res, array, 0);

		charArrayToStrings[array] = res;
	}
	pthread_mutex_unlock(&stringsMutex);
	return res;
}

J3ObjectHandle* J3::nameToString(const vmkit::Name* name) {
	pthread_mutex_lock(&stringsMutex);
	J3ObjectHandle* res = nameToCharArrays[name];
	if(!res) {
		J3ObjectHandle* prev = J3Thread::get()->tell();
		res = initialClassLoader->globalReferences()->add(J3ObjectHandle::doNewArray(charArrayClass, name->length()));
		J3Thread::get()->restore(prev);

		for(uint32_t i=0; i<name->length(); i++)
			res->setCharAt(i, name->cStr()[i]);
		nameToCharArrays[name] = res;
	}
	pthread_mutex_unlock(&stringsMutex);
	return arrayToString(res);
}

J3ObjectHandle* J3::utfToString(const char* name) {
	return nameToString(names()->get(name));
}

void J3::classCastException() {
	internalError(L"implement me: class cast exception");
}

void J3::nullPointerException() {
	internalError(L"implement me: null pointer exception");
}

void J3::noClassDefFoundError(J3Class* cl) {
	internalError(L"NoClassDefFoundError: %ls (%p - %p)", cl->name()->cStr(), cl, cl->name());
}

void J3::noSuchMethodError(const wchar_t* msg, J3Class* cl, const vmkit::Name* name, const vmkit::Name* sign) {
	internalError(L"%ls: %ls::%ls %ls", msg, cl->name()->cStr(), name->cStr(), sign->cStr());
}

void J3::classFormatError(J3Class* cl, const wchar_t* reason, ...) {
	wchar_t buf[65536];
	va_list va;
	va_start(va, reason);
	vswprintf(buf, 65536, reason, va);
	va_end(va);
	internalError(L"ClassFormatError in '%ls' caused by '%ls'", cl->name()->cStr(), buf);
}

void J3::linkageError(J3Method* method) {
	internalError(L"unable to find native method '%ls::%ls%ls'", method->cl()->name()->cStr(), method->name()->cStr(), method->sign()->cStr());
}

void J3::arrayStoreException() {
	internalError(L"array store exception");
}

void J3::arrayIndexOutOfBoundsException() {
	internalError(L"array bound check exception");
}

void J3::vinternalError(const wchar_t* msg, va_list va) {
	wchar_t buf[65536];
	vswprintf(buf, 65536, msg, va);
	fprintf(stderr, "Internal error: %ls\n", buf);
	printStackTrace();
	abort();
}

void J3::printStackTrace() {
	vmkit::StackWalker walker;

	while(walker.next()) {
		vmkit::Safepoint* sf = J3Thread::get()->vm()->getSafepoint(walker.ip());

		if(sf) {
			J3Method* m = ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
			fprintf(stderr, "    in %ls %ls::%ls index %d\n", m->sign()->cStr(), m->cl()->name()->cStr(), m->name()->cStr(),
							sf->sourceIndex());
		} else {
			Dl_info info;
			
			if(dladdr(walker.ip(), &info)) {
				int   status;
				const char* demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
				const char* name = demangled ? demangled : info.dli_sname;
				fprintf(stderr, "    in %s + %lu\n", name, (uintptr_t)walker.ip() - (uintptr_t)info.dli_saddr);
			} else {
				fprintf(stderr, "    in %p\n", walker.ip()); 
			}
		}
	}
}
