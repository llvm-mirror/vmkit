#ifdef SERVICE

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Target/TargetJITInfo.h"

#include "../lib/ExecutionEngine/JIT/JIT.h"

#include "JavaThread.h"
#include "JavaJIT.h"
#include "Jnjvm.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/JIT.h"

#include "signal.h"

using namespace jnjvm;

#if defined(__MACH__) && !defined(__i386__)
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
#endif



static void throwInlineStoppedBundleException() {
  void** addr = (void**)__builtin_frame_address(0);
  JavaThread* th = JavaThread::get();
  FRAME_IP(addr) = (void**)th->replacedEIPs[--th->eipIndex];
  th->throwException(th->ServiceException);
}

static void throwStoppedBundleException() {
  JavaThread* th = JavaThread::get();
  th->throwException(th->ServiceException);
}


static mvm::LockNormal lock;
static mvm::Cond cond;
static mvm::Thread* initiator = 0;
static bool Finished = true;

void terminationHandler(int) {
  void** addr = (void**)__builtin_frame_address(0);
  mvm::Thread* th = mvm::Thread::get();
  JnjvmClassLoader* stoppedBundle = 
    (JnjvmClassLoader*)(th->stoppingService->CU);
  void* baseSP = th->baseSP;
  bool inStack = false;
  while (addr && addr < baseSP && addr < addr[0]) {
    addr = (void**)addr[0];
    void** ptr = (void**)FRAME_IP(addr);
    JavaMethod* meth = JavaJIT::IPToJavaMethod(ptr);
    if (meth) {
      if (meth->classDef->classLoader == stoppedBundle) {
        inStack = true;
        JavaThread* th = JavaThread::get();
        th->replacedEIPs[th->eipIndex++] = FRAME_IP(addr);
        FRAME_IP(addr) = (void**)(uintptr_t)throwInlineStoppedBundleException;
      }
    }
  }

  // If the malicious bundle is in the stack, interrupt the thread.
  if (inStack) {
    JavaThread* th = JavaThread::get();
    th->lock.lock();
    th->interruptFlag = 1;

    // here we could also raise a signal for interrupting I/O
    if (th->state == JavaThread::StateWaiting) {
      th->state = JavaThread::StateInterrupted;
      th->varcond.signal();
    }
  
    th->lock.unlock();

  }

  if (mvm::Thread::get() != initiator) {
    lock.lock();
    while (!Finished)
      cond.wait(&lock);
    lock.unlock();
  }
}



void Jnjvm::stopService() {
  
  lock.lock();
  while (!Finished)
    cond.wait(&lock);

  Finished = false;
  lock.unlock();

  JnjvmClassLoader* bundle = (JnjvmClassLoader*)CU;
  bundle->getIsolate()->status = 1;
  mvm::Thread* th = mvm::Thread::get();
  th->stoppingService = this;
  initiator = th;
  for(mvm::Thread* cur = (mvm::Thread*)th->next(); cur != th;
      cur = (mvm::Thread*)cur->next()) {
    mvm::VirtualMachine* executingVM = cur->MyVM;
    assert(executingVM && "Thread with no VM!");
    cur->stoppingService = this;
    uint32 res = cur->kill(SIGUSR1);
    assert(res == 0);

  }
  
  // I have to do it too!
  terminationHandler(0);
   
  llvm::TargetJITInfo& TJI = ((llvm::JIT*)mvm::MvmModule::executionEngine)->getJITInfo();
  for (ClassMap::iterator i = bundle->getClasses()->map.begin(), e = bundle->getClasses()->map.end();
       i!= e; ++i) {
    Class* cl = i->second->asClass();

    if (cl) {
      for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
        if (cl->virtualMethods[i].code) {
          TJI.replaceMachineCodeForFunction(cl->virtualMethods[i].code, (void*)(uintptr_t)throwStoppedBundleException);
        }
      }
    
      for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
        if (cl->staticMethods[i].code) {
          TJI.replaceMachineCodeForFunction(cl->staticMethods[i].code, (void*)(uintptr_t)throwStoppedBundleException);
        }
      }
    }
  }

  lock.lock();
  Finished = true;
  cond.broadcast();
  lock.unlock();
}

#endif
