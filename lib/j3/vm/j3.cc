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
	J3Lib::processOptions(this);

	vmkit::ThreadAllocator::initialize(sizeof(J3Thread), options()->stackSize);

	J3Thread* thread = new J3ThreadBootstrap(this);

	vmkitBootstrap(thread, options()->selfBitCodePath);	

	if(options()->debugLifeCycle)
		fprintf(stderr, "  VM terminate\n");
}

void J3::run() {
#define defJavaConstantName(name, id) \
	name = names()->get(id);
	onJavaConstantNames(defJavaConstantName)
#undef defJavaConstantName

	introspect();

	vmkit::BumpAllocator* loaderAllocator = vmkit::BumpAllocator::create();
	initialClassLoader = new(loaderAllocator) J3InitialClassLoader(loaderAllocator);

	vmkit::BumpAllocator* a = initialClassLoader->allocator();

#define defPrimitive(name, ctype, llvmtype, scale)											\
	type##name = new(a) J3Primitive(initialClassLoader, J3Cst::ID_##name, llvm::Type::get##llvmtype##Ty(llvmContext()), scale);
	onJavaTypes(defPrimitive)
#undef defPrimitive

	clinitSign = initialClassLoader->getSignature(0, clinitSignName);

#define z_class(clName)                      initialClassLoader->loadClass(names()->get(clName))
#define z_method(access, cl, name, signature) cl->findMethod(access, name, initialClassLoader->getSignature(cl, signature))
#define z_field(access, cl, name, type)      cl->findField(access, names()->get(name), type)


	nbArrayInterfaces          = 2;
	arrayInterfaces            = (J3Class**)initialClassLoader->allocator()->allocate(2*sizeof(J3Type*));
	arrayInterfaces[0]         = z_class("java.lang.Cloneable");
	arrayInterfaces[1]         = z_class("java.io.Serializable");

	charArrayClass             = typeCharacter->getArray();
	objectClass                = z_class("java.lang.Object");
	objectClass->resolve();
	
	stringClass                = z_class("java.lang.String");
	stringClassInit            = z_method(0, stringClass, initName, names()->get("([CZ)V"));
	stringClassValue           = z_field(0, stringClass, "value", charArrayClass);

	classClass                 = z_class("java.lang.Class");
	J3Field vmData[] = { J3Field(J3Cst::ACC_PRIVATE, names()->get("** vmData **"), typeLong) };
	classClass->resolve(vmData, 1);
	classClassInit             = z_method(0, classClass, initName, names()->get("()V"));
	classClassVMData           = classClass->findField(0, vmData[0].name(), vmData[0].type());

	classLoaderClass           = z_class("java.lang.ClassLoader");
	classLoaderClass->resolve(vmData, 1);
	classLoaderClassVMData     = classLoaderClass->findField(0, vmData[0].name(), vmData[0].type());
	classLoaderClassLoadClass  = z_method(0, classLoaderClass, names()->get("loadClass"), 
																				names()->get("(Ljava/lang/String;)Ljava/lang/Class;"));
	classLoaderClassGetSystemClassLoader = z_method(J3Cst::ACC_STATIC, 
																									classLoaderClass,
																									names()->get("getSystemClassLoader"),
																									names()->get("()Ljava/lang/ClassLoader;"));

	threadClass                = z_class("java.lang.Thread");
	threadClassRun             = z_method(0, threadClass, names()->get("run"), names()->get("()V"));
	threadClassVMData          = z_field(0, threadClass, "eetop", typeLong);

	fieldClass                 = z_class("java.lang.reflect.Field");
	fieldClassClass            = z_field(0, fieldClass, "clazz", classClass);
	fieldClassSlot             = z_field(0, fieldClass, "slot", typeInteger);
	fieldClassAccess           = z_field(0, fieldClass, "modifiers", typeInteger);
	fieldClassInit             = z_method(0, fieldClass, initName, 
																				names()->get("(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Class;IILjava/lang/String;[B)V"));

	constructorClass           = z_class("java.lang.reflect.Constructor");
	constructorClassClass      = z_field(0, constructorClass, "clazz", classClass);
	constructorClassSlot       = z_field(0, constructorClass, "slot", typeInteger);
	constructorClassInit       = z_method(0, constructorClass, initName,
																			names()->get("(Ljava/lang/Class;[Ljava/lang/Class;[Ljava/lang/Class;IILjava/lang/String;[B[B)V"));

	methodClass                = z_class("java.lang.reflect.Method");
	methodClassClass           = z_field(0, methodClass, "clazz", classClass);
	methodClassSlot            = z_field(0, methodClass, "slot", typeInteger);
	methodClassInit            = z_method(0, methodClass, initName,
		names()->get("(Ljava/lang/Class;Ljava/lang/String;[Ljava/lang/Class;Ljava/lang/Class;[Ljava/lang/Class;IILjava/lang/String;[B[B[B)V"));

	throwableClassBacktrace    = z_field(0, z_class("java.lang.Throwable"), "backtrace", objectClass);

	stackTraceElementClass     = z_class("java.lang.StackTraceElement");
	stackTraceElementClassInit = z_method(0, stackTraceElementClass, initName,
																				names()->get("(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V"));

#define defJavaClassPrimitive(name, ctype, llvmtype, scale)	\
	type##name->defineJavaClass("java/lang/"#name);
	onJavaTypes(defJavaClassPrimitive)
#undef defJavaClassPrimitive

	if(options()->debugLifeCycle)
		fprintf(stderr, "  Bootstrap the system library\n");

	J3Lib::bootstrap(this);

	if(options()->isAOT)
		compileApplication();
	else
		runApplication();
}

void J3::compileApplication() {
	if(options()->debugLifeCycle)
		fprintf(stderr, "  Find the class loader\n");

	J3ClassLoader* loader = J3ClassLoader::nativeClassLoader(z_method(J3Cst::ACC_STATIC, z_class("java.lang.ClassLoader"),
																																		names()->get("getSystemClassLoader"),
																																		names()->get("()Ljava/lang/ClassLoader;"))->invokeStatic().valObject);


	if(options()->mainClass)
		J3::internalError("compiling a single class is not yet supported");
	else {
		J3Class*        jarFileClass = z_class("java.util.jar.JarFile");
		J3ObjectHandle* jarFile = J3ObjectHandle::doNewObject(jarFileClass);
		z_method(0, jarFileClass, initName, names()->get("(Ljava/lang/String;)V"))->invokeSpecial(jarFile, utfToString(options()->jarFile));
		J3ObjectHandle* it = z_method(0, jarFileClass, names()->get("entries"), 
																	names()->get("()Ljava/util/Enumeration;"))->invokeVirtual(jarFile).valObject;
		J3Method* hasMore = z_method(0, it->vt()->type()->asObjectType(), names()->get("hasMoreElements"), names()->get("()Z"));
		J3Method* nextEl = z_method(0, it->vt()->type()->asObjectType(), names()->get("nextElement"), names()->get("()Ljava/lang/Object;"));
		J3Method* getName = z_method(0, z_class("java.util.zip.ZipEntry"), names()->get("getName"), names()->get("()Ljava/lang/String;"));

		while(hasMore->invokeVirtual(it).valBoolean) {
			const vmkit::Name* name = stringToName(getName->invokeVirtual(nextEl->invokeVirtual(it).valObject).valObject);
			
			if(strcmp(name->cStr() + name->length() - 6, ".class") == 0) {
				char buf[name->length() - 5];
				memcpy(buf, name->cStr(), name->length() - 6);
				buf[name->length()-6] = 0;
				J3Class* c = loader->getTypeFromQualified(0, buf)->asClass();

				c->aotCompile();
			}
		}
	}
}

void J3::runApplication() {
	if(options()->debugLifeCycle)
		fprintf(stderr, "  Find the application\n");

	//options()->debugExecute = 0;

#define LM_CLASS 1
#define LM_JAR   2

	uint32_t    mode;
	const char* what;

	if(options()->mainClass) {
		mode = LM_CLASS;
		what = options()->mainClass;
	} else {
		mode = LM_JAR;
		what = options()->jarFile;
	}

	J3Class* main = J3ObjectType::nativeClass(z_method(J3Cst::ACC_STATIC,
																										 z_class("sun.launcher.LauncherHelper"),
																										 names()->get("checkAndLoadMain"),
																										 names()->get("(ZILjava/lang/String;)Ljava/lang/Class;"))
																						->invokeStatic(1, mode, utfToString(what)).valObject)->asClass();


	if(options()->debugLifeCycle)
		fprintf(stderr, "  Launch the application\n");

	J3ObjectHandle* args = J3ObjectHandle::doNewArray(stringClass->getArray(), options()->nbArgs);
	for(uint32_t i=0; i<options()->nbArgs; i++)
		args->setObjectAt(i, utfToString(options()->args[i]));

	main
		->findMethod(J3Cst::ACC_STATIC, names()->get("main"), initialClassLoader->getSignature(0, names()->get("([Ljava/lang/String;)V")))
		->invokeStatic(args);
}

JNIEnv* J3::jniEnv() {
	return J3Thread::get()->jniEnv();
}

const vmkit::Name* J3::qualifiedToBinaryName(const char* type, size_t length) {
	if(length == -1)
		length = strlen(type);
	char buf[length+1];
	for(size_t i=0; i<length; i++) {
		char c = type[i];
		buf[i] = c == '/' ? '.' : c;
	}
	buf[length] = 0;
	return names()->get(buf);
}

J3ObjectHandle* J3::arrayToString(J3ObjectHandle* array, bool doPush) {
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
	return doPush ? J3Thread::get()->push(res) : res;
}

J3ObjectHandle* J3::nameToString(const vmkit::Name* name, bool doPush) {
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
		res->setRegionCharacter(0, buf, 0, pos);
		J3Thread::get()->restore(prev);

		nameToCharArrays[name] = res;
	}
	pthread_mutex_unlock(&stringsMutex);
	return arrayToString(res, doPush);
}

J3ObjectHandle* J3::utfToString(const char* name, bool doPush) {
	return nameToString(names()->get(name), doPush);
}

const vmkit::Name* J3::arrayToName(J3ObjectHandle* array) {
	char copy[J3Utf16Decoder::maxSize(array)];
	J3Utf16Decoder::decode(array, copy);
	return names()->get(copy);
}

const vmkit::Name* J3::stringToName(J3ObjectHandle* str) {
	return arrayToName(str->getObject(stringClassValue));
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

void J3::noSuchMethodError(const char* msg, J3ObjectType* cl, const vmkit::Name* name, J3Signature* signature) {
	internalError("%s: %s::%s %s", msg, cl->name()->cStr(), name->cStr(), signature->name()->cStr());
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
	internalError("unable to find native method '%s::%s%s'", 
								method->cl()->name()->cStr(), 
								method->name()->cStr(), 
								method->signature()->name()->cStr());
}

void J3::outOfMemoryError() {
	internalError("out of memory error");
}

void J3::negativeArraySizeException(int32_t length) {
	internalError("negative array size exception: %ld", length);
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

void J3::illegalArgumentException(const char* msg) {
	internalError("illegal argument exception: %s", msg);
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
			fprintf(stderr, "    in %s::%s%s index %d\n", m->cl()->name()->cStr(), m->name()->cStr(),
							m->signature()->name()->cStr(), sf->sourceIndex());
		} else {
			Dl_info info;
			
			if(dladdr((void*)((uintptr_t)walker.ip()-1), &info)) {
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

void J3::uncatchedException(void* e) {
	J3Thread* thread = J3Thread::get();
	J3ObjectHandle* prev = thread->tell();

	try {
		J3ObjectHandle* excp = thread->push((J3Object*)e);

		J3ObjectHandle* handler = 
			thread->javaThread()->vt()->type()->asObjectType()
			->findMethod(0, 
									 names()->get("getUncaughtExceptionHandler"),
									 initialClassLoader->getSignature(0, names()->get("()Ljava/lang/Thread$UncaughtExceptionHandler;")))
			->invokeVirtual(thread->javaThread())
			.valObject;

		handler->vt()->type()->asObjectType()
			->findMethod(0,
									 names()->get("uncaughtException"),
									 initialClassLoader->getSignature(0, names()->get("(Ljava/lang/Thread;Ljava/lang/Throwable;)V")))
			->invokeVirtual(handler, thread->javaThread(), excp);

	} catch(void* e2) {
		fprintf(stderr, "Fatal: double exception %p and %p\n", e, e2);
	}
	thread->restore(prev);
}

void J3::forceSymbolDefinition() {
	J3ArrayObject a; a.length(); /* J3ArrayObject */
	J3LockRecord* l = new J3LockRecord(); /* J3LockRecord */
	try {
		throw (void*)0;
	} catch(void* e) {
	}
}
