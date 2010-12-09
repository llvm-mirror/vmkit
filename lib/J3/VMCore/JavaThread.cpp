//===--------- JavaThread.cpp - Java thread description -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"


using namespace j3;

JavaThread::JavaThread(Jnjvm* vm, mvm::Thread* mut) : mvm::VMThreadData(vm, mut) {
  jniEnv = vm->jniEnv;
  localJNIRefs = new JNILocalReferences();
  currentAddedReferences = NULL;
  javaThread = NULL;
  vmThread = NULL;
}

JavaThread* JavaThread::j3Thread(mvm::Thread* mut) {
	return (JavaThread*)mut->vmData;
}

JavaThread* JavaThread::associate(Jnjvm* vm, mvm::Thread* mut) {
	JavaThread *th   = new JavaThread(vm, mut);
	mut->allVmsData[vm->vmID] = th;
	mut->attach(vm);
	return th;
}


void JavaThread::initialise(JavaObject* thread, JavaObject* vmth) {
  llvm_gcroot(thread, 0);
  llvm_gcroot(vmth, 0);
  javaThread = thread;
  vmThread = vmth;
}

JavaThread::~JavaThread() {
  delete localJNIRefs;
}

void JavaThread::startJNI() {
  // Interesting, but no need to do anything.
}

void JavaThread::endJNI() {
  localJNIRefs->removeJNIReferences(this, *currentAddedReferences);
  mut->endUnknownFrame();
   
  // Go back to cooperative mode.
  mut->leaveUncooperativeCode();
}

uint32 JavaThread::getJavaFrameContext(void** buffer) {
  mvm::StackWalker Walker(mut);
  uint32 i = 0;

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->isHighLevelMethod()) {
      JavaMethod* M = (JavaMethod*)MI->MetaInfo;
      buffer[i++] = M;
    }
    ++Walker;
  }
  return i;
}

JavaMethod* JavaThread::getCallingMethodLevel(uint32 level) {
  mvm::StackWalker Walker(mut);
  uint32 index = 0;

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->isHighLevelMethod()) {
      if (index == level) {
        return (JavaMethod*)MI->MetaInfo;
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
  
  mvm::StackWalker Walker(mut);

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->isHighLevelMethod() == 1) {
      JavaMethod* meth = (JavaMethod*)MI->MetaInfo;
      JnjvmClassLoader* loader = meth->classDef->classLoader;
      obj = loader->getJavaClassLoader();
      if (obj) return obj;
    }
    ++Walker;
  }
  return 0;
}


void JavaThread::printJavaBacktrace() {
  mvm::StackWalker Walker(mut);

  while (mvm::MethodInfo* MI = Walker.get()) {
    if (MI->isHighLevelMethod())
      MI->print(Walker.ip, Walker.addr);
    ++Walker;
  }
}

mvm::gc** JNILocalReferences::addJNIReference(JavaThread* th, mvm::gc* obj) {
  llvm_gcroot(obj, 0);
  
  if (length == MAXIMUM_REFERENCES) {
    JNILocalReferences* next = new JNILocalReferences();
    th->localJNIRefs = next;
    next->prev = this;
    return next->addJNIReference(th, obj);
  } else {
    localReferences[length] = obj;
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
