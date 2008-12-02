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

#include "execinfo.h"

using namespace jnjvm;

static void* StackTrace[256];

// PrintStackTrace - In the case of a program crash or fault, print out a stack
// trace so that the user has an indication of why and where we died.
//
// On glibc systems we have the 'backtrace' function, which works nicely, but
// doesn't demangle symbols.  
static void PrintStackTrace() {
  // Use backtrace() to output a backtrace on Linux systems with glibc.
  int depth = backtrace(StackTrace, 256);
  backtrace_symbols_fd(StackTrace, depth, STDERR_FILENO);
}



static void throwStoppedBundleException() {
  void** addr = (void**)__builtin_frame_address(0);
  fprintf(stderr, "in stopped %p\n", addr);
  JavaJIT::printBacktrace();
  PrintStackTrace();
  fprintf(stderr, "OK\n");
  JavaThread* th = JavaThread::get();
  th->throwException(th->ServiceException);
}


static JnjvmClassLoader* stoppedBundle;
static mvm::LockNormal lock;
static mvm::Cond cond;
static mvm::Thread* initiator;


#if defined(__MACH__) && !defined(__i386__)
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
#endif


void terminationHandler(int) {
  void** addr = (void**)__builtin_frame_address(0);
  void* baseSP = mvm::Thread::get()->baseSP;
  while (addr && addr < baseSP && addr < addr[0]) {
    addr = (void**)addr[0];
    void** ptr = (void**)FRAME_IP(addr);
    JavaMethod* meth = JavaJIT::IPToJavaMethod(ptr);
    if (meth) {
      if (meth->classDef->classLoader == stoppedBundle) {
        fprintf(stderr, "Je change %p!\n", FRAME_IP(addr));
        JavaJIT::printBacktrace();
        FRAME_IP(addr) = (void**)(uintptr_t)throwStoppedBundleException;
      }
    }
  }
  
  addr = (void**)__builtin_frame_address(0);
  while (addr && addr < baseSP && addr < addr[0]) {
    addr = (void**)addr[0];
    void** ptr = (void**)FRAME_IP(addr);
    JavaMethod* meth = JavaJIT::IPToJavaMethod(ptr);
    if (meth) {
      if (meth->classDef->classLoader != stoppedBundle) {
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
      break;
    }
  }

  if (mvm::Thread::get() != initiator) {
    lock.lock();
    while (stoppedBundle)
      cond.wait(&lock);
    lock.unlock();
  } else {
    fprintf(stderr, "Je suis l'initiateur, je quitte\n");
  }
}



void Jnjvm::stopService() {
 
  JnjvmClassLoader* bundle = (JnjvmClassLoader*)CU;
  bundle->getIsolate()->status = 1;
  stoppedBundle = bundle;
  mvm::Thread* th = mvm::Thread::get();
  th->MyVM->memoryLimit = ~0;
  initiator = th;
  fprintf(stderr, "I am %p\n", th);
  for(mvm::Thread* cur = (mvm::Thread*)th->next(); cur != th;
      cur = (mvm::Thread*)cur->next()) {
    mvm::VirtualMachine* executingVM = cur->MyVM;
    assert(executingVM && "Thread with no VM!");
    fprintf(stderr, "Killing th %p\n", cur);
    uint32 res = cur->kill(SIGUSR1);
    assert(res == 0);

  }
  
  fprintf(stderr, "Doing it\n");
  // I have to do it too!
  terminationHandler(0);
  fprintf(stderr, "OK! bon ben moi je m'en vais\n");
  /* 
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
  stoppedBundle = 0;
  cond.broadcast();*/
}

#endif
