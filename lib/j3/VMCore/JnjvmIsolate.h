
#include <sys/mman.h>
#include <llvm/Support/Memory.h>
#include <cerrno>

#include "VmkitGC.h"
#include "Jnjvm.h"
#include "JavaClass.h"
#include "JavaUpcalls.h"
#include "VMStaticInstance.h"
#include "j3/JavaJITCompiler.h"
#include "j3/jni.h"


extern "C" uint32_t j3SetIsolate(uint32_t isolateID, uint32_t* currentIsolateID);

extern "C" void CalledStoppedIsolateMethod(void* methodIP) __attribute__((noinline));
extern "C" void ReturnedToStoppedIsolateMethod(void* methodIP) __attribute__((noinline));

extern "C" void StoppedIsolate_Redirect_ReturnToDeadMethod() __attribute__((naked, noreturn, noinline));

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod() __attribute__((naked, noreturn, noinline));
extern "C" void StoppedIsolate_Redirect_CallToDeadMethod_End() __attribute__((naked, noreturn, noinline));

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop() __attribute__((naked, noreturn, noinline));
extern "C" void StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_End() __attribute__((naked, noreturn, noinline));

#define StoppedIsolate_Redirect_CallToDeadMethod_CodeSize	\
	((size_t)((intptr_t)StoppedIsolate_Redirect_CallToDeadMethod_End - (intptr_t)StoppedIsolate_Redirect_CallToDeadMethod))

#define StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_CodeSize	\
	((size_t)((intptr_t)StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_End - (intptr_t)StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop))
