#include "vmkit/config.h"

#include "j3/j3.h"
#include "j3/j3lib.h"
#include "j3/j3config.h"
#include "j3/j3constants.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"
#include "j3/j3class.h"
#include "j3/j3thread.h"

#include <dlfcn.h>

using namespace j3;

#ifdef LINUX_OS
#define OPENJDK_LIBPATH OPENJDK_HOME"jre/lib/amd64"
#else
#define OPENJDK_LIBPATH OPENJDK_HOME"jre/lib"
#endif

static const char* rtjar = OPENJDK_HOME"jre/lib/rt.jar";

void J3Lib::bootstrap(J3* vm) {
	J3ObjectHandle* prev = J3Thread::get()->tell();

#define z_signature(id) vm->initialClassLoader->getSignature(0, vm->names()->get(id))

	J3Class*  threadGroupClass = vm->initialClassLoader->loadClass(vm->names()->get("java/lang/ThreadGroup"));
	J3Method* sysThreadGroupInit = threadGroupClass->findMethod(0, vm->initName, z_signature("()V"));
	J3ObjectHandle* sysThreadGroup = J3ObjectHandle::doNewObject(threadGroupClass);
	sysThreadGroupInit->invokeSpecial(sysThreadGroup);

	J3Method* appThreadGroupInit = threadGroupClass->findMethod(0, vm->initName, z_signature("(Ljava/lang/ThreadGroup;Ljava/lang/String;)V"));
	J3ObjectHandle* appThreadGroup = J3ObjectHandle::doNewObject(threadGroupClass);
	appThreadGroupInit->invokeSpecial(appThreadGroup, sysThreadGroup, vm->utfToString("main", 0));

	J3Method* threadInit = vm->threadClass->findMethod(0, vm->initName, z_signature("(Ljava/lang/ThreadGroup;Ljava/lang/String;)V"));
	J3ObjectHandle* mainThread = J3ObjectHandle::doNewObject(vm->threadClass);

	J3Thread::get()->assocJavaThread(mainThread);
	mainThread->setInteger(vm->threadClass->findField(0, vm->names()->get("priority"), vm->typeInteger), 5);

	threadInit->invokeSpecial(mainThread, appThreadGroup, vm->utfToString("main", 0));
						
	vm->initialClassLoader
		->loadClass(vm->names()->get("java/lang/System"))
		->findMethod(J3Cst::ACC_STATIC, vm->names()->get("initializeSystemClass"), z_signature("()V"))
		->invokeStatic();

	J3Thread::get()->restore(prev);
}

const char* J3Lib::systemClassesArchives() {
	return rtjar;
}

const char* J3Lib::systemLibraryPath() {
	return OPENJDK_LIBPATH;
}

const char* J3Lib::extDirs() {
	return OPENJDK_HOME"jre/lib/ext";
}

int J3Lib::loadSystemLibraries(std::vector<void*, vmkit::StdAllocator<void*> >* nativeLibraries) {
	/* JavaRuntimeSupport checks for a symbol defined in this library */
	void* h0 = dlopen(OPENJDK_LIBPATH"/libinstrument"SHLIBEXT, RTLD_LAZY | RTLD_GLOBAL);
	void* handle = dlopen(OPENJDK_LIBPATH"/libjava"SHLIBEXT, RTLD_LAZY | RTLD_LOCAL);
	
	if(!handle || !h0) {
		fprintf(stderr, "Fatal: unable to find java system library: %s\n", dlerror());
		abort();
	}

	nativeLibraries->push_back(handle);

	return 0;
}

