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
#include "j3/j3field.h"
#include "j3/j3utf16.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

#include "vmkit/safepoint.h"
#include "vmkit/system.h"

using namespace j3;

vmkit::T_ptr_less_t<J3ObjectHandle*> J3::charArrayLess;
vmkit::T_ptr_less_t<llvm::FunctionType*> J3::llvmFunctionTypeLess;

J3::J3(vmkit::BumpAllocator* allocator) : 
	VMKit(allocator),
	nameToCharArrays(vmkit::Name::less, allocator),
	charArrayToStrings(charArrayLess, allocator),
	_names(allocator),
	monitorManager(allocator),
	llvmSignatures(llvmFunctionTypeLess, allocator) {

#define defJavaConstantName(name, id) \
	name = names()->get(id);
	onJavaConstantNames(defJavaConstantName)
#undef defJavaConstantName

	pthread_mutex_init(&stringsMutex, 0);
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
	typeJ3LayoutPtr         = llvm::PointerType::getUnqual(introspectType("class.j3::J3Layout"));
	typeJ3ObjectType        = introspectType("class.j3::J3ObjectType");
	typeJ3ObjectTypePtr     = llvm::PointerType::getUnqual(typeJ3ObjectType);
	typeJ3Thread            = llvm::PointerType::getUnqual(introspectType("class.j3::J3Thread"));
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
	typeJ3LockRecord        = introspectType("class.j3::J3LockRecord");

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

#define defPrimitive(name, ctype, llvmtype, scale)											\
	type##name = new(a) J3Primitive(initialClassLoader, J3Cst::ID_##name, llvm::Type::get##llvmtype##Ty(llvmContext()), scale);
	onJavaTypes(defPrimitive)
#undef defPrimitive

#define z_class(clName)                      initialClassLoader->loadClass(names()->get(clName))
#define z_method(access, cl, name, sign)     initialClassLoader->method(access, cl, name, sign)
#define z_field(access, cl, name, type)      J3Cst::isStatic(access)	\
			? cl->findStaticField(names()->get(name), type)									\
			: cl->findVirtualField(names()->get(name), type);


	nbArrayInterfaces    = 2;
	arrayInterfaces      = (J3Type**)initialClassLoader->allocator()->allocate(2*sizeof(J3Type*));
	arrayInterfaces[0]   = z_class("java/lang/Cloneable");
	arrayInterfaces[1]   = z_class("java/io/Serializable");

	charArrayClass           = typeChar->getArray();
	objectClass              = z_class("java/lang/Object");
	objectClass->resolve();
	
	stringClass              = z_class("java/lang/String");
	stringClassInit          = z_method(0, stringClass, initName, names()->get("([CZ)V"));
	stringClassValue         = z_field(0, stringClass, "value", charArrayClass);

	classClass               = z_class("java/lang/Class");
	J3Field hf(J3Cst::ACC_PRIVATE, names()->get("** vmData **"), typeLong);
	classClass->resolve(&hf, 1);
	classClassInit           = z_method(0, classClass, initName, names()->get("()V"));
	classClassVMData         = classClass->findVirtualField(hf.name(), hf.type());

	threadClass              = z_class("java/lang/Thread");
	threadClassRun           = z_method(0, threadClass, names()->get("run"), names()->get("()V"));
	threadClassVMData        = initialClassLoader->loadClass(names()->get("java/lang/Thread"))
		->findVirtualField(names()->get("eetop"), typeLong);

	fieldClass               = z_class("java/lang/reflect/Field");
	fieldClassClass          = z_field(0, fieldClass, "clazz", classClass);
	fieldClassSlot           = z_field(0, fieldClass, "slot", typeInteger);
	fieldClassAccess         = z_field(0, fieldClass, "modifiers", typeInteger);
	fieldClassInit           = z_method(0, fieldClass, initName, 
																			names()->get("(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Class;IILjava/lang/String;[B)V"));

	constructorClass         = z_class("java/lang/reflect/Constructor");
	constructorClassClass    = z_field(0, constructorClass, "clazz", classClass);
	constructorClassSlot     = z_field(0, constructorClass, "slot", typeInteger);
	constructorClassInit     = z_method(0, constructorClass, initName,
																			names()->get("(Ljava/lang/Class;[Ljava/lang/Class;[Ljava/lang/Class;IILjava/lang/String;[B[B)V"));

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

		stringClassInit->invokeSpecial(res, array, 0);

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
		uint16_t buf[name->length()];
		size_t pos = 0;
		J3Utf16Encoder encoder(name);

		while(!encoder.isEof())
			buf[pos++] = encoder.nextUtf16();

		res = initialClassLoader->globalReferences()->add(J3ObjectHandle::doNewArray(charArrayClass, pos));
		res->setRegionChar(0, buf, 0, pos);
		J3Thread::get()->restore(prev);

		nameToCharArrays[name] = res;
	}
	pthread_mutex_unlock(&stringsMutex);
	return arrayToString(res);
}

J3ObjectHandle* J3::utfToString(const char* name) {
	return nameToString(names()->get(name));
}

void J3::classCastException() {
	internalError("implement me: class cast exception");
}

void J3::nullPointerException() {
	internalError("implement me: null pointer exception");
}

void J3::classNotFoundException(const vmkit::Name* name) {
	internalError("ClassNotFoundException: %s", name);
}

void J3::noClassDefFoundError(const vmkit::Name* name) {
	internalError("NoClassDefFoundError: %s", name);
}

void J3::noSuchMethodError(const char* msg, J3ObjectType* cl, const vmkit::Name* name, const vmkit::Name* sign) {
	internalError("%s: %s::%s %s", msg, cl->name()->cStr(), name->cStr(), sign->cStr());
}

void J3::noSuchFieldError(const char* msg, J3ObjectType* cl, const vmkit::Name* name, J3Type* type) {
	internalError("%s: %s::%s %s", msg, cl->name()->cStr(), name->cStr(), type->name()->cStr());
}

void J3::classFormatError(J3ObjectType* cl, const char* reason, ...) {
	char buf[65536];
	va_list va;
	va_start(va, reason);
	vsnprintf(buf, 65536, reason, va);
	va_end(va);
	internalError("ClassFormatError in '%s' caused by '%s'", cl->name()->cStr(), buf);
}

void J3::linkageError(J3Method* method) {
	internalError("unable to find native method '%s::%s%s'", method->cl()->name()->cStr(), method->name()->cStr(), method->sign()->cStr());
}

void J3::arrayStoreException() {
	internalError("array store exception");
}

void J3::arrayIndexOutOfBoundsException() {
	internalError("array bound check exception");
}

void J3::illegalMonitorStateException() {
	internalError("illegal monitor state exception");
}

void J3::vinternalError(const char* msg, va_list va) {
	char buf[65536];
	vsnprintf(buf, 65536, msg, va);
	fprintf(stderr, "Internal error: %s\n", buf);
	printStackTrace();
	//exit(1);
	abort();
}

void J3::printStackTrace() {
	vmkit::StackWalker walker;

	while(walker.next()) {
		vmkit::Safepoint* sf = J3Thread::get()->vm()->getSafepoint(walker.ip());

		if(sf) {
			J3Method* m = ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
			fprintf(stderr, "    in %s %s::%s index %d\n", m->sign()->cStr(), m->cl()->name()->cStr(), m->name()->cStr(),
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

void J3::forceSymbolDefinition() {
	J3ArrayObject a; a.length(); /* J3ArrayObject */
	J3LockRecord* l = new J3LockRecord(); /* J3LockRecord */
	try {
		throw (void*)0;
	} catch(void* e) {
	}
}


