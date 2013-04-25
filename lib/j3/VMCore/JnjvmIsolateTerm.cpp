
#include "JnjvmIsolate.h"

using namespace j3;


void Jnjvm::invalidateAllClassesInIsolate(isolate_id_t isolateID)
{
	// Mark all classes as invalid in the given isolate
	ClassMap* classes = RunningIsolates[isolateID].loader->getClasses();
//	classes->lock.lock();

	for (isolate_id_t runningIsolateID = 0; runningIsolateID < NR_ISOLATES; ++runningIsolateID) {
		if (RunningIsolates[runningIsolateID].state == IsolateFree) continue;

		classes = RunningIsolates[runningIsolateID].loader->getClasses();
		for (ClassMap::iterator cli = classes->map.begin(), cliEnd = classes->map.end(); cli != cliEnd; ++cli) {
			if (!cli->second->isClass()) continue;

			Class *cl = cli->second->asClass();
			for (isolate_id_t deniedIsolateID = 0; deniedIsolateID < NR_ISOLATES; ++deniedIsolateID) {
				if (!(RunningIsolates[deniedIsolateID].state & IsolateDenyExecution)) continue;

				TaskClassMirror& tcm = cl->getTaskClassMirror(deniedIsolateID);
				tcm.initialized = false;
				tcm.status = loaded;
			}
		}
	}
//	classes->lock.unlock();
}

void Jnjvm::denyIsolateExecutionInThread(isolate_id_t isolateID, JavaThread& thread)
{
	if (!thread.isVmkitThread()) return;	// This is not a Java thread

	removeExceptionHandlersInThread(isolateID, thread);

	// We look at every pair of methods (native and Java methods).
	vmkit::StackWalker CallerWalker(&thread);

	if (!CallerWalker.isValid())
		return;	// No methods (native/Java) to inspect in this stack

	vmkit::StackWalker CalledWalker(CallerWalker);
	++CallerWalker;

	if (!CallerWalker.isValid()) return;	// One frame in the stack (is this normal? possible?)

	// Loop over pairs of method frames (caller/called) in the stack
	for (vmkit::StackWalkerState state = vmkit::StackWalkerInvalid;
		(state = CallerWalker.getState()) >= vmkit::StackWalkerValid;
		CalledWalker = CallerWalker, ++CallerWalker)
	{
		if (state != vmkit::StackWalkerValidMetadata)
			continue;	// The caller method is not a Java method

		const vmkit::FrameInfo* CallerFrame = CallerWalker.get();
		if (isolateID != getFrameIsolateID(*CallerFrame))
			continue;	// The caller method does not belong to the terminating isolate

		// The caller method is a Java method belonging to the terminating isolate.
		vmkit::StackWalker LastJavaCallerWalker(CallerWalker);

		// Look for the next Java methods.
		CallerWalker.reportOnlyMetadataEnabledFrames(true);
		for (++CallerWalker; ; LastJavaCallerWalker = CallerWalker, ++CallerWalker) {
			if ((state = CallerWalker.getState()) == vmkit::StackWalkerInvalid) {
				// No Java methods calling this method, only native methods.
				CallerWalker = LastJavaCallerWalker;
				break;
			}

			// We found a Java caller method
			CallerFrame = CallerWalker.get();
			if (isolateID != getFrameIsolateID(*CallerFrame)) {
				// The Java caller method does not belong to the terminating isolate.
				// Make CallerWalker point at the last frame before that foreign Java method frame.
				vmkit::StackWalker ForeignCaller(CallerWalker);
				CallerWalker = LastJavaCallerWalker;
				CallerWalker.reportOnlyMetadataEnabledFrames(false);

				for (++CallerWalker;
					CallerWalker != ForeignCaller;
					LastJavaCallerWalker = CallerWalker, ++CallerWalker);

				CallerWalker = LastJavaCallerWalker;
				break;
			}

			// We found another Java caller method belonging to the terminating isolate.
			// Continue scanning...
		}
		CallerWalker.reportOnlyMetadataEnabledFrames(false);

		// CalledWalker                     : called frame, native or NOT belonging to the terminating isolate.
		// (CalledWalker + 1)...CallerWalker: caller frames, native or belonging to the terminating isolate.
		denyIsolateExecutionInMethodFrame(CallerWalker, CalledWalker);

		// We are sure that the next method frame is either native or a Java method frame
		// not belonging to the terminating isolate.
		++CallerWalker;
	}

	// If the thread was running code in the isolate to kill, set its current isolate ID
	// to the dead isolate ID, it should throw an exception as soon as it accesses isolate ID
	if (thread.getIsolateID() == isolateID)
		thread.markRunningDeadIsolate();
}

isolate_id_t Jnjvm::getFrameIsolateID(const vmkit::FrameInfo& frame) const
{
	assert(frame.Metadata && "Method frame has no metadata.");
	return getFrameIsolateID(*(const JavaMethod*)frame.Metadata);
}

isolate_id_t Jnjvm::getFrameIsolateID(const JavaMethod& method) const
{
	const JnjvmClassLoader* loader = method.classDef->classLoader;
	return (loader == loader->bootstrapLoader) ? 0 : loader->getIsolateID();
}

void Jnjvm::denyIsolateExecutionInMethodFrame(const vmkit::StackWalker& CallerWalker, vmkit::StackWalker& CalledWalker)
{
	denyMethodExecution(CallerWalker, CalledWalker);

	// Patch the return address of the given method frame in order to return to the address of
	// StoppedIsolate_RedirectMethodCodeToException when it executes the ret instruction.
	vmkit::StackWalker walker(CalledWalker);
	++walker;
	walker.updateReturnAddress((void*)(intptr_t)(&StoppedIsolate_Redirect_ReturnToDeadMethod));

	// Patch the thread stack frames to exclude the given method frames, so that further scanning of the stack does
	// not reveal those method frames.
	walker.updateCallerFrameAddress(CallerWalker.getCallerCallFrame());
}

void Jnjvm::redirectMethodProlog(void* methodAddress, void* redirectCode, size_t codeSize)
{
	llvm::sys::MemoryBlock methodProlog(methodAddress, codeSize);
	llvm::sys::Memory::setWritable(methodProlog);

	memcpy(methodAddress, redirectCode, codeSize);

	llvm::sys::Memory::InvalidateInstructionCache(methodAddress, codeSize);
}

void Jnjvm::removeExceptionHandlersInThread(isolate_id_t isolateID, JavaThread& thread)
{
	// Remove all exception handlers which run in the given isolate.
	// These exception handlers might exist in a method that was loaded by the isolate class loader itself,
	// and might be a method that does not modify the current isolate ID (e.g. a Java runtime method) and
	// which consequently run in the isolate of its caller.

	vmkit::ExceptionBuffer *CurExceptBuffer, *CalledExceptBuffer = NULL;
	for (CurExceptBuffer = thread.lastExceptionBuffer;
		CurExceptBuffer != NULL;
		CurExceptBuffer = CurExceptBuffer->getPrevious())
	{
		if (isolateID != CurExceptBuffer->getHandlerIsolateID()) {
			CalledExceptBuffer = CurExceptBuffer;
			continue;
		}

		if (JavaMethod* method = (JavaMethod*)thread.MyVM->IPToFrameInfo(CurExceptBuffer->getHandlerMethod())->Metadata) {
			std::cout << '[' << thread << "] Disabling exception handler inside method=" <<
				*method->classDef->name << '.' << *method->name <<
				" in isolateID=" << getFrameIsolateID(*method) << std::endl;
		}

		// Skip the exception buffer
		if (!CalledExceptBuffer)
			thread.lastExceptionBuffer = CurExceptBuffer->getPrevious();
		else
			CalledExceptBuffer->setPrevious(CurExceptBuffer->getPrevious());
	}
}
/*
void Jnjvm::removeExceptionHandlersInMethodFrames(JavaThread& thread, const vmkit::StackWalker& CallerWalker, vmkit::StackWalker& CalledWalker)
{
	// Remove all exception handlers in the given methods
	vmkit::ExceptionBuffer *CurExceptBuffer, *CalledExceptBuffer = NULL;
	bool MoreFramesToCheck = true, skippedExceptHandler = false;
	vmkit::StackWalker CalledMarkerWalker(CalledWalker);
	++CalledMarkerWalker;	// Skip the called method

	for (CurExceptBuffer = thread.lastExceptionBuffer;
		CurExceptBuffer != NULL;
		CalledExceptBuffer = skippedExceptHandler ? CalledExceptBuffer : CurExceptBuffer,
		CurExceptBuffer = CurExceptBuffer->getPrevious())
	{
		vmkit::FrameInfo* ExceptHandlerMethodFrame;
		JavaMethod* ExceptHandlerMethodInfo;
		vmkit::StackWalker walker(CalledMarkerWalker);
		skippedExceptHandler = false;

		void* ExceptHandlerMethodAddr = CurExceptBuffer->getHandlerMethod();
		if (!ExceptHandlerMethodAddr) continue;

		ExceptHandlerMethodFrame = thread.MyVM->IPToFrameInfo(ExceptHandlerMethodAddr);
		if (!ExceptHandlerMethodFrame) continue;

		ExceptHandlerMethodInfo = (JavaMethod*)ExceptHandlerMethodFrame->Metadata;
		if (!ExceptHandlerMethodInfo) continue;

		walker.reportOnlyMetadataEnabledFrames(false);

		for (bool inLastCaller = false; !inLastCaller; ++walker) {
			inLastCaller = (walker == CallerWalker);

			vmkit::FrameInfo* DeadMethodFrame = walker.get();
			if (!DeadMethodFrame) continue;

			JavaMethod* DeadMethodInfo = (JavaMethod*)DeadMethodFrame->Metadata;
			if (!DeadMethodInfo) continue;
			if (DeadMethodInfo != ExceptHandlerMethodInfo) continue;

			std::cout << '[' << thread << "] Disabling exception handler inside method=" <<
				*DeadMethodInfo->classDef->name << '.' << *DeadMethodInfo->name <<
				" in isolateID=" << getFrameIsolateID(*DeadMethodInfo) << std::endl;

			// Skip the exception buffer
			if (!CalledExceptBuffer)
				thread.lastExceptionBuffer = CurExceptBuffer->getPrevious();
			else
				CalledExceptBuffer->setPrevious(CurExceptBuffer->getPrevious());

			// This called method frame exception buffer is now skipped, don't check it again.
			CalledMarkerWalker = walker;
			++CalledMarkerWalker;

			skippedExceptHandler = true;
			MoreFramesToCheck = !inLastCaller;	// Still have method frames to check?
			break;
		}

		if (!MoreFramesToCheck)
			break;
	}
}
*/
void Jnjvm::denyMethodExecution(const vmkit::StackWalker& CallerWalker, vmkit::StackWalker& CalledWalker)
{
	vmkit::StackWalker walker(CalledWalker);
	do {
		++walker;
		if (walker.getState() < vmkit::StackWalkerValidMetadata) continue;

		denyMethodExecution(*((JavaMethod*)walker.get()->Metadata));
	} while (walker != CallerWalker);
}

void Jnjvm::denyMethodExecution(JavaMethod& method)
{
	void *redirectCode;
	size_t redirectCodeSize;
	if (isActivatorStopMethod(method)) {
		redirectCode = (void*)(intptr_t)StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop;
		redirectCodeSize = StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_CodeSize;
	} else {
		redirectCode = (void*)(intptr_t)StoppedIsolate_Redirect_CallToDeadMethod;
		redirectCodeSize = StoppedIsolate_Redirect_CallToDeadMethod_CodeSize;
	}

	if (method.code == redirectCode)
		return;	// Already denied, nothing to do

	if (method.code != NULL) {	// Method was generated, overwrite its prolog code in memory
		std::cout << "Patching prolog of method[generated,non-custom]=" <<
			*method.classDef->name << '.' << *method.name <<
			" in isolateID=" << getFrameIsolateID(method) << std::endl;

		redirectMethodProlog(method.code, redirectCode, redirectCodeSize);

		method.code = redirectCode;		// Make the function point to the redirection code.
		return;
	}

	if (!method.isCustomizable) {
		std::cout << "Patching prolog of method[not-generated,non-custom]=" <<
			*method.classDef->name << '.' << *method.name <<
			" in isolateID=" << getFrameIsolateID(method) << std::endl;

		method.code = redirectCode;		// Make the function point to the redirection code.
		return;
	}

	// NOTE:
	// We must NOT generate code here, because we might cause a dead lock while trying to obtain
	// the LLVM-IR lock via protectIR().

	// Avoid any further customizations to this method
	JavaJITCompiler* compiler = static_cast<JavaJITCompiler*>(method.classDef->classLoader->getCompiler());
	LLVMMethodInfo* methodInfo = compiler->getMethodInfo(&method);

	method.isCustomizable = false;
	methodInfo->isCustomizable = false;

	// Patch all the customized versions of the method.
	LLVMMethodInfo::customizedVersionsType* methodVersions = methodInfo->getCustomizedVersions();
	for (LLVMMethodInfo::customizedVersionsIterator methodCode = methodVersions->begin(),
		methodCodeEnd = methodVersions->end();
		methodCode != methodCodeEnd;
		++methodCode)
	{
		std::cout << "Patching prolog of method[generated,custom]=" <<
			*method.classDef->name << '.' << *method.name <<
			" customizedFor=" << *methodCode->first->name <<
			" in isolateID=" << getFrameIsolateID(method) << std::endl;

		void *code = compiler->executionEngine->getPointerToGlobal(methodCode->second);
		redirectMethodProlog(code, redirectCode, redirectCodeSize);
	}

	method.code = redirectCode;		// Make the function point to the redirection code.
}

bool Jnjvm::isActivatorStopMethod(JavaMethod& method) const
{
	/*
		WARNING: We have made enough checks to be sure this method is:
		public void stop(BundleContext bundleContext) throws Exception

		However, we must also check that the class of this method is the Activator of the bundle.
		This would require calling org.osgi.framework.Bundle.getHeaders() and finding the
		"Bundle-Activator" meta-data to compare it with method.classDef.
	*/
	if (!isPublic(method.access) || isStatic(method.access)) return false;

	const Signdef* sign = method.getSignature();
	if ((sign->nbArguments != 1) || !sign->getReturnType()->isVoid()) return false;

	JnjvmBootstrapLoader* bootstrapLoader = method.classDef->classLoader->bootstrapLoader;
	return	method.name->equals(bootstrapLoader->stop)
			&& (**sign->getArgumentsType()).keyName->equals(bootstrapLoader->org_osgi_framework_BundleContext);
}

void Jnjvm::denyClassExecution(isolate_id_t isolateID, CommonClass& ccl)
{
	if ((ccl.classLoader->getIsolateID() != isolateID)
		|| ccl.isPrimitive() || ccl.isArray() || ccl.isInterface())
		return;

	if (ccl.super)
		denyClassExecution(isolateID, *ccl.super);	// Deny the super class, if any

	Class& cl = *ccl.asClass();
	for (size_t i=0; i < cl.nbInnerClasses; ++i)
		denyClassExecution(isolateID, *cl.innerClasses[i]);	// Deny inner classes, if any

	std::cout << "Denying class execution: " << *cl.name << std::endl;

	for (size_t i=0; i < cl.nbStaticMethods; ++i)
		denyMethodExecution(cl.staticMethods[i]);	// Deny static class methods

	for (size_t i=0; i < cl.nbVirtualMethods; ++i)
		denyMethodExecution(cl.virtualMethods[i]);	// Deny virtual object methods
}

extern "C" void CalledStoppedIsolateMethod(void* methodIP)
{
	JavaThread *thread = JavaThread::get();
	JavaMethod* method = NULL;
	thread->cleanUpOnDeadIsolateBeforeException(&methodIP, &method);

	if (method) {
		std::cout << '[' << *thread << "] DeadIsolateException(ReturnedToStoppedIsolateMethod) in method=" <<
			*method->classDef->name << '.' << *method->name << std::endl;
	}

	thread->MyVM->deadIsolateException(methodIP);
	UNREACHABLE();
}

extern "C" void ReturnedToStoppedIsolateMethod(void* methodIP)
{
	JavaThread *thread = JavaThread::get();
	JavaMethod* method = NULL;
	thread->cleanUpOnDeadIsolateBeforeException(&methodIP, &method);

	if (method) {
		std::cout << '[' << *thread << "] DeadIsolateException(ReturnedToStoppedIsolateMethod) in method=" <<
			*method->classDef->name << '.' << *method->name << std::endl;
	}

	thread->MyVM->deadIsolateException(methodIP);
	UNREACHABLE();
}

void Jnjvm::deadIsolateException(void* methodIP, bool immediate)
{
	//error(upcalls->DeadIsolateException, upcalls->InitDeadIsolateException, str, immediate);
	error(
		upcalls->InterruptedException, upcalls->InitInterruptedException,
		"Called method is implemented by a bundle that was stopped.",
		immediate);
}

#if defined(ARCH_X86) && defined(LINUX_OS)

/*
This code treats the case where M0 calls M1 which calls M2 (Mi are Java methods) where:
- M1 belongs to the terminating isolate, and
- M0 and M2 do not.

M2 will continue running as expected. But when it returns, instead of jumping to M1, it comes here.
In fact, M2 should have its on-stack return address patched to jump here.
So, this code will run in the stack frame of M1. This code will adjust the stack frame of M1 then
simulate a call to throw an exception. The adjustment made will make it seem as if M0 thrown the
exception directly.

In short, this code transforms: M0 > M1 > M2
						  into: M0 > throw exception

NOTICE:
* The exception thrower function should never return to its caller, except to exception handlers.
* This code supposes that the calling convention of LLVM-generated code is cdecl. The stdcall calling
  convention requires the called method to know the number of parameters it receives and to pop them
  out on return (instruction: ret N), this would prohibit us from writing a generic code to patch the
  stack, and would require dynamic code generation, or another way of doing things...
* This code must be position-independent because it might be copied elsewhere, so no relative
  offsets should be generated. Careful choosing instructions.
*/

extern "C" void StoppedIsolate_Redirect_ReturnToDeadMethod()
{
	asm volatile (
		"mov %ebp, %esp								\n"	// Free all locals
		"pop %ebp									\n"	// caller EBP ==> EBP
		"push (%esp)								\n"	// Arg0 = ReturnAddress
		// Copy the full function address into the register to avoid generating an offset-based call instruction.
		"mov $ReturnedToStoppedIsolateMethod, %eax	\n"
		"jmp %eax									\n"	// simulate a call from caller
	);
}

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod()
{
	asm volatile (
		"push (%esp)											\n"	// Arg0 = ReturnAddress
		// Copy the full function address into the register to avoid generating an offset-based call instruction.
		"mov $CalledStoppedIsolateMethod, %eax					\n"
		"jmp %eax												\n"	// simulate a call from caller
		"StoppedIsolate_Redirect_CallToDeadMethod_End:			\n"	// Mark the end of code

		".globl StoppedIsolate_Redirect_CallToDeadMethod_End	\n"	// Reserve space for the label address
	);
}

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop()
{
	asm volatile (
		"ret																\n"	// Return directly to the caller
		"StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_End:		\n"// Mark the end of code

		".globl StoppedIsolate_Redirect_CallToDeadMethod_ActivatorStop_End	\n"// Reserve space for the label address
	);
}

/*
// NOTE: This works more correctly than __builtin_frame_address because this does NOT require stack frame to be setup.
extern "C" void* StoppedIsolate_GetEIP()
{
	asm volatile (
		"mov (%esp), %eax	\n"
		"ret				\n"
	);
}

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod_Redirect()
{
	asm volatile (
		"sub $StoppedIsolate_Redirect_CallToDeadMethod_Code, %eax	\n"
		"add $StoppedIsolate_Redirect_CallToDeadMethod_Data, %eax	\n"
		"mov (%eax), %eax											\n"

		"push %ebp													\n"
		"push %eax													\n"
		"mov $CalledStoppedIsolateMethod, %eax						\n"
		"call %eax													\n"
		);
}

extern "C" void StoppedIsolate_Redirect_CallToDeadMethod()
{
	asm volatile (
		"mov $StoppedIsolate_GetEIP, %eax						\n"
		"call %eax												\n"

		"StoppedIsolate_Redirect_CallToDeadMethod_Code:			\n"
		"mov $StoppedIsolate_Redirect_CallToDeadMethod_Redirect, %ebx	\n"
		"jmp %ebx												\n"

		"StoppedIsolate_Redirect_CallToDeadMethod_Data:			\n"
		".fill 4												\n"// Enough to store a (void*)
		"StoppedIsolate_Redirect_CallToDeadMethod_End:			\n"

		".globl StoppedIsolate_Redirect_CallToDeadMethod_End	\n"
	);
}
*/

#else
#error "Sorry. Only Linux x86 is currently supported."
#endif
