
#include "JnjvmIsolate.h"

using namespace j3;


extern "C" jint Java_ijvm_isolate_vm_IJVM_getObjectIsolateID(JavaObject* object)
{
	llvm_gcroot(object, 0);
	const UserCommonClass* ccl = JavaObject::getClass(object);
	isolate_id_t isolateID = ccl->classLoader->getIsolateID();
	return isolateID;
}

extern "C" void Java_ijvm_isolate_vm_IJVM_disableIsolates(ArrayLong *isolateID, jboolean denyIsolateExecution, jboolean resetIsolateReferences)
{
	size_t isolateCount = ArrayLong::getSize(isolateID);
	isolate_id_t* list = new isolate_id_t[isolateCount];

	for (size_t i=0; i < isolateCount; ++i)
		list[i] = (isolate_id_t)ArrayLong::getElement(isolateID, i);

	((Jnjvm*)vmkit::Thread::get()->MyVM)->disableIsolates(list, isolateCount, denyIsolateExecution, resetIsolateReferences);

	delete [] list;
}

void Jnjvm::disableIsolates(isolate_id_t* isolateID, size_t isolateCount, bool denyIsolateExecution, bool resetIsolateReferences)
{
	if (!isolateID || !isolateCount || (!denyIsolateExecution && !resetIsolateReferences)) return;	// Nothing to do

	for (size_t i=0; i < isolateCount; ++i) {
		if (isolateID[i] == 0)
			illegalArgumentException("isolateID must not be zero");
	}

	{
		vmkit::LockGuard lg(IsolateLock);
		for (size_t i=0; i < isolateCount; ++i) {
			RunningIsolates[isolateID[i]].state |=
				(resetIsolateReferences ? IsolateResetReferences : 0) |
				(denyIsolateExecution ? IsolateDenyExecution : 0);
		}
	}

	vmkit::Collector::collect();
}

void Jnjvm::collectIsolates()
{
//	vmkit::LockGuard lg(IsolateLock);
	for (isolate_id_t isolateID = 0; isolateID < NR_ISOLATES; ++isolateID)
		collectIsolate(isolateID);
}

void Jnjvm::collectIsolate(isolate_id_t isolateID)
{
	if (!(RunningIsolates[isolateID].state & IsolateDenyExecution)) return;

	// Deny current isolate execution in running threads
	vmkit::Thread *thisThread = vmkit::Thread::get();
	for (vmkit::Thread* th = (vmkit::Thread*)thisThread->next(); th != thisThread; th = (vmkit::Thread*)th->next())
		denyIsolateExecutionInThread(isolateID, *(JavaThread*)th);

	// Deny future isolate execution
	ClassMap* classes = RunningIsolates[isolateID].loader->getClasses();
//	classes->lock.lock();
	for (ClassMap::iterator cl = classes->map.begin(), clEnd = classes->map.end(); cl != clEnd; ++cl)
		denyClassExecution(isolateID, *cl->second);
//	classes->lock.unlock();

	invalidateAllClassesInIsolate(isolateID);
}

void Jnjvm::printCallStack(const vmkit::StackWalker& walker)
{
	vmkit::FrameInfo* FI = NULL;
	JavaThread* thread = (JavaThread*)walker.getScannedThread();

	std::cerr << '[' << *thread << "] Call stack trace from call frame=" << walker.getCallFrame() << std::endl;
	for (vmkit::StackWalker w(walker); (FI = w.get()) != NULL; ++w)
		this->printMethod(FI, w.getReturnAddress(), w.getCallFrame());
	std::cerr << std::endl;
}

void Jnjvm::loadIsolate(JnjvmClassLoader *loader)
{
  JavaObject* obj = NULL;
  JavaObject* javaLoader = NULL;
  llvm_gcroot(obj, 0);
  llvm_gcroot(javaLoader, 0);

  assert((bootstrapLoader->upcalls->newString != NULL) && "bootstrap class loader not initialized");

#define LOAD_CLASS(cl) \
  cl->resolveClass(); \
  cl->initialiseClass(this);

  LOAD_CLASS(upcalls->newString);

  // The initialization code of the classes initialized below may require
  // to get the Java thread, so we create the Java thread object first.
  LOAD_CLASS(upcalls->newThread);
  LOAD_CLASS(upcalls->newVMThread);
  LOAD_CLASS(upcalls->threadGroup);

  LOAD_CLASS(upcalls->newClass);
  LOAD_CLASS(upcalls->newConstructor);
  LOAD_CLASS(upcalls->newField);
  LOAD_CLASS(upcalls->newMethod);
  LOAD_CLASS(upcalls->newStackTraceElement);
  LOAD_CLASS(upcalls->boolClass);
  LOAD_CLASS(upcalls->byteClass);
  LOAD_CLASS(upcalls->charClass);
  LOAD_CLASS(upcalls->shortClass);
  LOAD_CLASS(upcalls->intClass);
  LOAD_CLASS(upcalls->longClass);
  LOAD_CLASS(upcalls->floatClass);
  LOAD_CLASS(upcalls->doubleClass);
  LOAD_CLASS(upcalls->InvocationTargetException);
  LOAD_CLASS(upcalls->ArrayStoreException);
  LOAD_CLASS(upcalls->ClassCastException);
  LOAD_CLASS(upcalls->IllegalMonitorStateException);
  LOAD_CLASS(upcalls->IllegalArgumentException);
  LOAD_CLASS(upcalls->InterruptedException);
  LOAD_CLASS(upcalls->IndexOutOfBoundsException);
  LOAD_CLASS(upcalls->ArrayIndexOutOfBoundsException);
  LOAD_CLASS(upcalls->NegativeArraySizeException);
  LOAD_CLASS(upcalls->NullPointerException);
  LOAD_CLASS(upcalls->SecurityException);
  LOAD_CLASS(upcalls->ClassFormatError);
  LOAD_CLASS(upcalls->ClassCircularityError);
  LOAD_CLASS(upcalls->NoClassDefFoundError);
  LOAD_CLASS(upcalls->UnsupportedClassVersionError);
  LOAD_CLASS(upcalls->NoSuchFieldError);
  LOAD_CLASS(upcalls->NoSuchMethodError);
  LOAD_CLASS(upcalls->InstantiationError);
  LOAD_CLASS(upcalls->IllegalAccessError);
  LOAD_CLASS(upcalls->IllegalAccessException);
  LOAD_CLASS(upcalls->VerifyError);
  LOAD_CLASS(upcalls->ExceptionInInitializerError);
  LOAD_CLASS(upcalls->LinkageError);
  LOAD_CLASS(upcalls->AbstractMethodError);
  LOAD_CLASS(upcalls->UnsatisfiedLinkError);
  LOAD_CLASS(upcalls->InternalError);
  LOAD_CLASS(upcalls->OutOfMemoryError);
  LOAD_CLASS(upcalls->StackOverflowError);
  LOAD_CLASS(upcalls->UnknownError);
  LOAD_CLASS(upcalls->ClassNotFoundException);
  LOAD_CLASS(upcalls->ArithmeticException);
  LOAD_CLASS(upcalls->InstantiationException);
  LOAD_CLASS(upcalls->SystemClass);
  LOAD_CLASS(upcalls->cloneableClass);
  LOAD_CLASS(upcalls->CloneNotSupportedException);
#undef LOAD_CLASS

  // Implementation-specific end-of-bootstrap initialization
  upcalls->InitializeSystem(this);

  obj = JavaThread::get()->currentThread();
  javaLoader = loader->getJavaClassLoader();
  upcalls->setContextClassLoader->invokeIntSpecial(this, upcalls->newThread, obj, &javaLoader);

  // load and initialize math since it is responsible for dlopen'ing
  // libjavalang.so and we are optimizing some math operations
  UserCommonClass* math = bootstrapLoader->loadName(bootstrapLoader->mathName, true, true, NULL);
  math->asClass()->initialiseClass(this);
}

int Jnjvm::allocateNextFreeIsolateID(JnjvmClassLoader* loader, isolate_id_t *isolateID)
{
	isolate_id_t i = 0;
	{
		vmkit::LockGuard lg(IsolateLock);
		for (; (i < NR_ISOLATES) && (RunningIsolates[i].state != IsolateFree); ++i);

		if (i < NR_ISOLATES) {
			RunningIsolates[i].state = IsolateRunning;
			RunningIsolates[i].loader = loader;

			if (isolateID != NULL) *isolateID = i;
		}
	}

	assert((i < NR_ISOLATES) && "Not enough isolate slots");
	return (i < NR_ISOLATES) ? 0 : ENOENT;
}

void Jnjvm::freeIsolateID(isolate_id_t isolateID)
{
	vmkit::LockGuard lg(IsolateLock);
	RunningIsolates[isolateID].state = IsolateFree;
	RunningIsolates[isolateID].loader = NULL;

	std::cout << "Isolate " << isolateID << " unloaded from memory." << std::endl;
}
