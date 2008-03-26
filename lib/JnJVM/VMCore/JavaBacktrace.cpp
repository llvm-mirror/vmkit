//===---------- JavaBacktrace.cpp - Backtrace utilities -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <dlfcn.h>

#include "llvm/Function.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "mvm/JIT.h"
#include "mvm/Method.h"
#include "mvm/Object.h"

#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"

using namespace jnjvm;

extern "C" int** get_frame_pointer() {
/*
  int** fp = 0;
#if defined (__PPC__)
  asm volatile("1:\n"
    " mr %0, 1\n"
    : "=&r"(fp));
#else
  asm volatile("1:\n"
    " movl %%ebp, %0\n"
    : "=&r"(fp));
#endif
  return fp;
*/
  return (int**)__builtin_frame_address(0);
}

extern "C" int *debug_frame_ip(int **fp) {
#if defined(__MACH__) && !defined(__i386__)
	return fp[2];
#else
	return fp[1];
#endif
}

extern "C" int **debug_frame_caller_fp(int **fp) {
	return (int**)fp[0];
}


extern "C" int **debug_frame_caller_from_first_fp(int **fp) {
#if defined (__PPC__)
  return fp;
#else
  return fp;
#endif
}

extern "C" bool frame_end(int **fp) {
#if defined(__PPC__)
  return fp == 0;
#else
  return fp == 0;
#endif
}
    
extern "C" JavaMethod* ip_to_meth(int* begIp) {
  if (begIp) {
    const llvm::GlobalValue * glob = 
      mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
    if (glob) {
      if (llvm::isa<llvm::Function>(glob)) {
        mvm::Code* c = (mvm::Code*)begIp;
        mvm::Method* m = c->method();
        JavaMethod* meth = (JavaMethod*)m->definition();
        if (meth && meth->getVirtualTable() == JavaMethod::VT) {
          return meth;
        }
      }
    }
  }
  return 0;
}

#if 0
extern "C" int backtrace(void** ips, int size) {
  int** fp = get_frame_pointer();
  fp = debug_frame_caller_from_first_fp(fp);
  int i = 0;
  while ((!frame_end(fp)) && (debug_frame_ip(fp) != 0) && i < size) {
    int * ip = debug_frame_ip(fp);
    ips[i++] = (void*)ip;
    fp = debug_frame_caller_fp(fp);
  }
  return i;
}
#else
#include <execinfo.h>
#endif
extern "C" int backtrace_fp(int** ips, int size, int** fp) {
  fp = debug_frame_caller_from_first_fp(fp);
  int i = 0;
  while ((!frame_end(fp)) && (debug_frame_ip(fp) != 0) && i < size) {
    int * ip = debug_frame_ip(fp);
    ips[i++] = ip;
    fp = debug_frame_caller_fp(fp);
  }
  return i;
}

extern "C" void debug_backtrace(int **fp) {
  fp = debug_frame_caller_from_first_fp(fp);
  while ((!frame_end(fp)) && (debug_frame_ip(fp) != 0)) {
    int * ip = debug_frame_ip(fp);
    int *begIp = (int*)gc::begOf(ip);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          printf("; 0x%p in %s\n",  ip, 
                 ((llvm::Function*)glob)->getNameStr().c_str());
        } else JavaThread::get()->isolate->unknownError("in global variable?");
      } else printf("; 0x%p in stub\n",  ip);
    } else {
      Dl_info info;
      int res = dladdr(begIp, &info);
      if (res != 0) {
        printf("; 0x%p in %s\n", ip, info.dli_fname);
      }
    }
    fp = debug_frame_caller_fp(fp);
  }
}


void JavaJIT::printBacktrace() {
  int* ips[100];
  int real_size = backtrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    int *begIp = (int*)gc::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          JavaMethod* meth = (JavaMethod*)m->definition();
          if (meth) 
            printf("; 0x%p in %s\n",  ips[n - 1], meth->printString());
          else
            printf("; 0x%p in %s\n",  ips[n - 1], ((llvm::Function*)glob)->getNameStr().c_str());
        } else JavaThread::get()->isolate->unknownError("in global variable?");
      } else printf("; 0x%p in stub\n", ips[n - 1]);
    } else {
      Dl_info info;
      int res = dladdr(begIp, &info);
      if (res != 0) {
        printf("; 0x%p in %s\n",  ips[n - 1], info.dli_fname);
      } else {
        printf("; 0x%p in Unknown\n", ips[n - 1]);
      }
    }
  }
}




Class* JavaJIT::getCallingClass() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    int *begIp = (int*)gc::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          JavaMethod* meth = (JavaMethod*)m->definition();
          if (meth && i == 1) return meth->classDef;
          else ++i;
        } else {
          JavaThread::get()->isolate->unknownError("in global variable?");
        }
      }
    }
  }
  return 0;
}

Class* JavaJIT::getCallingClassWalker() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    int *begIp = (int*)gc::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          JavaMethod* meth = (JavaMethod*)m->definition();
          if (meth && i == 1) {
            return meth->classDef;
          } else ++i;
        } else {
          JavaThread::get()->isolate->unknownError("in global variable?");
        }
      }
    }
  }
  return 0;
}

JavaObject* JavaJIT::getCallingClassLoader() {
  Class* cl = getCallingClassWalker();
  if (!cl) return 0;
  else return cl->classLoader;
}
