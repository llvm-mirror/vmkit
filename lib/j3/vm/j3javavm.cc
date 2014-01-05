#include "j3/j3jni.h"
#include "j3/j3thread.h"
#include "j3/j3.h"

namespace j3 {

#define enterJVM() try {
#define leaveJVM() } catch(void* e) { J3Thread::get()->setPendingException(J3Thread::get()->push((J3Object*)e)); }

#define NYI() { J3Thread::get()->vm()->internalError("not yet implemented: '%s'", __PRETTY_FUNCTION__); }

jint DestroyJavaVM(JavaVM *vm) { enterJVM(); leaveJVM(); NYI(); }
jint AttachCurrentThread(JavaVM *vm, void **penv, void *args) { enterJVM(); leaveJVM(); NYI(); }
jint DetachCurrentThread(JavaVM *vm) { enterJVM(); leaveJVM(); NYI(); }
jint GetEnv(JavaVM *vm, void **penv, jint version) { enterJVM(); leaveJVM(); NYI(); }
jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *args) { enterJVM(); leaveJVM(); NYI(); }

struct JNIInvokeInterface_ javaVMTable = {
	0,
	0,
	0,
	DestroyJavaVM,
	AttachCurrentThread,
	DetachCurrentThread,
	GetEnv,
	AttachCurrentThreadAsDaemon,
};

}

