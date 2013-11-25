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

/// TODO optimize this function with iterative impl.
///
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
	return os << *thread.currentThread();
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
	permit = 0;
}

ParkLock::~ParkLock() {

}

inline void _OrderAccess_fence() {
    // Always use locked addl since mfence is sometimes expensive
	__sync_synchronize();
    //__asm__ volatile ("lock; addl $0,0(%%esp)" : : : "cc", "memory");
}

// Implementation of method park, see LockSupport.java
// time is in nanoseconds if !isAboslute, otherwise it is in milliseconds
void ParkLock::park(bool isAbsolute, int64_t time, JavaThread* thread) {
	 // Optional fast-path check:
	  // Return immediately if a permit is available.
	  if (permit) {
		  permit = false ;
		  //_OrderAccess_fence();//__sync_synchronize();//OrderAccess::fence();
	      return ;
	  }

	  // Optional optimization -- avoid state transitions if there's an interrupt pending.
	  // Check interrupt before trying to wait
	  if (thread->lockingThread.interruptFlag) {
	    return;
	  }

	  if (time < 0 || (isAbsolute && time == 0) ) { // don't wait at all
	    return;
	  }

	  // Enter safepoint region
	  // Beware of deadlocks such as 6317397.
	  // The per-thread Parker:: mutex is a classic leaf-lock.
	  // In particular a thread must never block on the Threads_lock while
	  // holding the Parker:: mutex.  If safepoints are pending both the
	  // the ThreadBlockInVM() CTOR and DTOR may grab Threads_lock.
	  //ThreadBlockInVM tbivm(jt);

	  // Don't wait if cannot get lock since interference arises from
	  // unblocking.  Also. check interrupt before trying wait
	  if (thread->lockingThread.interruptFlag || lock.tryLock() != 0) {
	    return;
	  }


	  int status ;
	  if (permit)  { // no wait needed
		permit = false;
		lock.unlock(thread);
		//_OrderAccess_fence();
		return;
	  }

	  //OSThreadWaitState osts(thread->osthread(), false /* not Object.wait() */);
	  //jt->set_suspend_equivalent();
	  // cleared by handle_special_suspend_equivalent_condition() or java_suspend_self()

	  if (time == 0) {
		  //thread->setState(vmkit::LockingThread::StateWaiting);
		  thread->setState(vmkit::LockingThread::StateWaiting);
		  //_OrderAccess_fence();
		  //thread->doYield = true;
		  cond.wait(&lock);
		  //thread->doYield = false;
		  //cond.myTimeWait(&lock, false, 1000000000);
	  } else {
		  thread->setState(vmkit::LockingThread::StateTimeWaiting);
		  cond.myTimeWait(&lock, isAbsolute, time);
	  }
	  thread->setState(vmkit::LockingThread::StateRunning);
	  //thread->setState(vmkit::LockingThread::StateRunning);
	  permit = false ;
	  //_OrderAccess_fence();
	  lock.unlock(thread);
	  // If externally suspended while waiting, re-suspend
	  //if (jt->handle_special_suspend_equivalent_condition()) {
	  //  jt->java_suspend_self();
	  //}
}

void ParkLock::unpark() {
	JavaThread* thread = (JavaThread*)vmkit::Thread::get();
	lock.lock();
	permit = true;
	//_OrderAccess_fence();
	cond.signal();
	lock.unlock(thread);
}

void ParkLock::interrupt() {
	//
	if (lock.tryLock() == 0) {
		JavaThread* thread = (JavaThread*)vmkit::Thread::get();
		cond.signal();

		lock.unlock(thread);
	}
	//_OrderAccess_fence();
}
