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
using namespace std;

JavaThread::JavaThread(vmkit::VirtualMachine* isolate) : MutatorThread() {
  MyVM = isolate;
  pendingException = NULL;
  jniEnv = ((Jnjvm*)isolate)->jniEnv;
  localJNIRefs = new JNILocalReferences();
  currentAddedReferences = NULL;
  javaThread = NULL;
  vmThread = NULL;
  state = vmkit::LockingThread::StateRunning;
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

void JavaThread::throwException(JavaObject* obj) {
  llvm_gcroot(obj, 0);
  JavaThread* th = JavaThread::get();
  assert(th->pendingException == 0 && "pending exception already there?");
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)&(th->pendingException), (gc*)obj);
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
      MyVM->printMethod(FI, Walker.ip, Walker.addr);
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

std::ostream& j3::operator << (std::ostream& os, const JavaThread& thread)
{
	JavaObject* jThread = NULL;
	JavaString* threadNameObj = NULL;
	llvm_gcroot(jThread, 0);
	llvm_gcroot(threadNameObj, 0);

	os << '[' << (void*)(&thread);

	Jnjvm* vm = thread.getJVM();
	jThread = thread.currentThread();
	if (vm && jThread) {
		threadNameObj = static_cast<JavaString*>(
			vm->upcalls->threadName->getInstanceObjectField(jThread));
		char *threadName = JavaString::strToAsciiz(threadNameObj);
		os << '(' << threadName << ')';
		delete [] threadName;
	}

	return os << ']';
}

void JavaThread::dump() const
{
	cerr << *this << endl;
}

void JavaThread::throwNullPointerException(word_t methodIP)
{
	if (!this->isVmkitThread())
		return vmkit::Thread::throwNullPointerException(methodIP);

	MyVM->nullPointerException();
	UNREACHABLE();
}

ParkLock::ParkLock() {
	permit = 1;
}

ParkLock::~ParkLock() {

}

// Implementation of method park, see LockSupport.java
// time is in nanoseconds if !isAboslute, otherwise it is in milliseconds
void ParkLock::park(bool isAbsolute, int64_t time) {
	JavaThread* thread = (JavaThread*)vmkit::Thread::get();
	lock.lock();
		if (permit == 0){
			permit = 1;
			__sync_synchronize();
			lock.unlock(thread);
			return;
		}
		if (isAbsolute && time == 0) {
			lock.unlock(thread);
			return;
		}
		if (time == 0) {
			thread->setState(vmkit::LockingThread::StateWaiting);
			permit = 2;
			__sync_synchronize();
			cond.wait(&lock);
			permit = 1;
		}
		else {
			thread->setState(vmkit::LockingThread::StateTimeWaiting);
			permit = 2;
			__sync_synchronize();
			cond.myTimeWait(&lock, isAbsolute, time);
			permit = 1;
		}
		thread->setState(vmkit::LockingThread::StateRunning);
		__sync_synchronize();
	lock.unlock(thread);
}

void ParkLock::unpark() {
	JavaThread* thread = (JavaThread*)vmkit::Thread::get();
	bool flag = false;
	lock.lock();
		if (permit != 0)
			flag = !__sync_bool_compare_and_swap(&permit, 1, 0);
	lock.unlock(thread);
	if (flag)
		cond.signal();
}

void ParkLock::interrupt() {
	cond.signal();
}
