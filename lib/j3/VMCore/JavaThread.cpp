//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "vmkit/Locks.h"
#include "vmkit/Thread.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaString.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"


using namespace j3;

JavaThread::JavaThread(Jnjvm* isolate) : MutatorThread() { 
  MyVM = isolate;
  JavaThread* th = JavaThread::get();
  if (th->isVmkitThread())
	  isolateID = th->getIsolateID();
  pendingException = NULL;
  jniEnv = isolate->jniEnv;
  localJNIRefs = new JNILocalReferences();
  currentAddedReferences = NULL;
  javaThread = NULL;
  vmThread = NULL;
}

void JavaThread::initialise(JavaObject* thread, JavaObject* vmth) {
  llvm_gcroot(thread, 0);
  llvm_gcroot(vmth, 0);
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)&javaThread, (gc*)thread);
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)&vmThread, (gc*)vmth);
}

JavaThread::~JavaThread() {
  delete localJNIRefs;
}

void JavaThread::throwException(JavaObject* obj, bool immediate) {
  llvm_gcroot(obj, 0);
  JavaThread* th = JavaThread::get();
  assert(th->pendingException == 0 && "pending exception already there?");
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)&(th->pendingException), (gc*)obj);
  if (immediate)
    th->internalThrowException();
}

void JavaThread::throwPendingException() {
  JavaThread* th = JavaThread::get();
  assert(th->pendingException);
  th->internalThrowException();
}

void JavaThread::startJNI() {
  // Interesting, but no need to do anything.
}

void JavaThread::endJNI() {
  localJNIRefs->removeJNIReferences(this, *currentAddedReferences);
  endUnknownFrame();
   
  // Go back to cooperative mode.
  leaveUncooperativeCode();
}

uint32 JavaThread::getJavaFrameContext(void** buffer) {
  vmkit::StackWalker Walker(this);
  uint32 i = 0;

  while (vmkit::FrameInfo* FI = Walker.get()) {
    if (FI->Metadata != NULL) {
      JavaMethod* M = (JavaMethod*)FI->Metadata;
      buffer[i++] = M;
    }
    ++Walker;
  }
  return i;
}

JavaMethod* JavaThread::getCallingMethodLevel(uint32 level) {
  vmkit::StackWalker Walker(this);
  uint32 index = 0;

  while (vmkit::FrameInfo* FI = Walker.get()) {
    if (FI->Metadata != NULL) {
      if (index == level) {
        return (JavaMethod*)FI->Metadata;
      }
      ++index;
    }
    ++Walker;
  }
  return 0;
}

UserClass* JavaThread::getCallingClassLevel(uint32 level) {
  JavaMethod* meth = getCallingMethodLevel(level);
  if (meth) return meth->classDef;
  return 0;
}

JavaObject* JavaThread::getNonNullClassLoader() {
  
  JavaObject* obj = 0;
  llvm_gcroot(obj, 0);
  
  vmkit::StackWalker Walker(this);

  while (vmkit::FrameInfo* FI = Walker.get()) {
    if (FI->Metadata != NULL) {
      JavaMethod* meth = (JavaMethod*)FI->Metadata;
      JnjvmClassLoader* loader = meth->classDef->classLoader;
      obj = loader->getJavaClassLoader();
      if (obj) return obj;
    }
    ++Walker;
  }
  return 0;
}


void JavaThread::printJavaBacktrace() {
  vmkit::StackWalker Walker(this);

  while (vmkit::FrameInfo* FI = Walker.get()) {
    if (FI->Metadata != NULL) {
      MyVM->printMethod(FI, Walker.getReturnAddress(), Walker.getCallFrame());
    }
    ++Walker;
  }
}

JavaObject** JNILocalReferences::addJNIReference(JavaThread* th,
                                                 JavaObject* obj) {
  llvm_gcroot(obj, 0);
  
  if (length == MAXIMUM_REFERENCES) {
    JNILocalReferences* next = new JNILocalReferences();
    th->localJNIRefs = next;
    next->prev = this;
    return next->addJNIReference(th, obj);
  } else {
    vmkit::Collector::objectReferenceNonHeapWriteBarrier(
        (gc**)&(localReferences[length]), (gc*)obj);
    return &localReferences[length++];
  }
}

void JNILocalReferences::removeJNIReferences(JavaThread* th, uint32_t num) {
    
  if (th->localJNIRefs != this) {
    delete th->localJNIRefs;
    th->localJNIRefs = this;
  }

  if (num > length) {
    assert(prev && "No prev and deleting too much local references");
    prev->removeJNIReferences(th, num - length);
  } else {
    length -= num;
  }
}

std::ostream& j3::operator << (std::ostream& os, JavaThread& thread)
{
	os << (void*)(&thread);

	Jnjvm* vm = thread.getJVM();
	JavaObject* jThread = thread.currentThread();
	if (vm && jThread) {
		JavaString* threadNameObj = static_cast<JavaString*>(
			vm->upcalls->threadName->getInstanceObjectField(jThread));
		char *threadName = JavaString::strToAsciiz(threadNameObj);
		os << '(' << threadName << ')';
		delete [] threadName;
	}

	return os << ',' << thread.getIsolateID();
}

void JavaThread::printStackTrace(int skip, int level_deep)
{
	JavaThread *thread = JavaThread::get();
	std::cerr << '[' << *thread << "] Call stack trace:" << std::endl;

	j3::Jnjvm *vm = thread->getJVM();
	vmkit::StackWalker Walker(thread);
	for (vmkit::FrameInfo* FI = NULL; (level_deep > 0) && ((FI = Walker.get()) != NULL); ++Walker) {
		if (!FI->Metadata) continue;
		if (skip > 0) {--skip; continue;}

		vm->printMethod(FI, Walker.getReturnAddress(), Walker.getCallFrame());
		--level_deep;
	}
}

void JavaThread::throwNullPointerException(void* methodIP) const
{
	if (!this->isVmkitThread())
		return vmkit::Thread::throwNullPointerException(methodIP);

	this->cleanUpOnDeadIsolateBeforeException(&methodIP);

	MyVM->nullPointerException();
	UNREACHABLE();
}

void JavaThread::throwDeadIsolateException() const
{
	if (this->runsDeadIsolate())
		const_cast<JavaThread*>(this)->setIsolateID(0);

	if (!this->isVmkitThread())
		return vmkit::Thread::throwDeadIsolateException();

	void *methodIP = StackWalker_getCallFrameAddress();
	methodIP = vmkit::StackWalker::getReturnAddressFromCallFrame(methodIP);

	MyVM->deadIsolateException(methodIP, true);
	UNREACHABLE();
}

extern "C" uint32_t j3SetIsolate(uint32_t isolateID, uint32_t* currentIsolateID);

void JavaThread::cleanUpOnDeadIsolateBeforeException(void** methodIP, JavaMethod** method) const
{
	vmkit::StackWalker walker(const_cast<JavaThread*>(this), true);
	vmkit::FrameInfo *FI = walker.get();

	if (!FI) {
		if (methodIP != NULL) *methodIP = NULL;
		if (method != NULL) *method = NULL;
		return;
	}

	JavaMethod* meth = (JavaMethod*)FI->Metadata;

	// Restore the current isolate ID to that of the caller
	isolate_id_t callerIsolateID = meth->classDef->classLoader->getIsolateID();
	j3SetIsolate(callerIsolateID, NULL);

	if (methodIP != NULL) *methodIP = FI->ReturnAddress;
	if (method != NULL) *method = meth;
}

void JavaThread::runAfterLeavingGarbageCollectorRendezVous()
{
	// Be sure to throw an exception if I am running in a dead isolate
	if (this->runsDeadIsolate())
		throwDeadIsolateException();
}
