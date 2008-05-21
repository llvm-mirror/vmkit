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

JavaMethod* JavaJIT::IPToJavaMethod(void* begIp) {
  mvm::Code* val = mvm::Code::getCodeFromPointer(begIp);
  if (val) {
    mvm::Method* m = val->method();
    mvm::Object* meth = m->definition();
    if (meth) {
      return (JavaMethod*)meth;
    }
  }
  return 0;
}

#if defined(__MACH__) && !defined(__i386__)
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
#endif

int JavaJIT::getBacktrace(void** stack, int size) {
  void** blah = (void**)__builtin_frame_address(1);
  int cpt = 0;
  void* baseSP = JavaThread::get()->baseSP;
  while (blah && cpt < size && blah < baseSP) {
    stack[cpt++] = (void**)FRAME_IP(blah);
    blah = (void**)blah[0];
  }
  return cpt;
}

void JavaJIT::printBacktrace() {
  int* ips[100];
  int real_size = getBacktrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth) {
        printf("; %p in %s\n",  ips[n - 1], meth->printString());
      } else if (m->llvmFunction) {
        printf("; %p in %s\n",  ips[n - 1],
                   m->llvmFunction->getNameStr().c_str());
      } else {
        printf("; %p in %s\n",  ips[n - 1], "stub (probably)");
      }
    } else {
      Dl_info info;
      int res = dladdr(ips[n++], &info);
      if (res != 0) {
        printf("; %p in %s\n",  ips[n - 1], info.dli_sname);
      } else {
        printf("; %p in Unknown\n", ips[n - 1]);
      }
    }
  }
}




Class* JavaJIT::getCallingClass() {
  int* ips[10];
  int real_size = getBacktrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth) {
        if (i == 1) {
          return ((JavaMethod*)meth)->classDef;
        } else {
          ++i;
        }
      }
    }
  }
  return 0;
}

Class* JavaJIT::getCallingClassWalker() {
  int* ips[10];
  int real_size = getBacktrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth) {
        if (i == 1) {
          return ((JavaMethod*)meth)->classDef;
        } else {
          ++i;
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
