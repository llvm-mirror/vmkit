//===------------ Backtrace.cpp - Backtrace utilities ---------------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <stdio.h>
#include <dlfcn.h>

#include "mvm/JIT.h"
#include "mvm/Method.h"
#include "mvm/Object.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMThread.h"

using namespace n3;

void CLIJit::printBacktrace() {
  int* ips[100];
  int real_size = mvm::jit::getBacktrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::jit::getCodeFromPointer(ips[n++]);
    if (code) {
      VMMethod* meth = (VMMethod*)code->getMetaInfo();
      if (meth) {
        printf("; %p in %s\n",  (void*)ips[n - 1], meth->printString());
      } else {
        printf("; %p in %s\n",  (void*)ips[n - 1], "unknown");
      }
    } else {
      Dl_info info;
      int res = dladdr(ips[n++], &info);
      if (res != 0) {
        printf("; %p in %s\n",  (void*)ips[n - 1], info.dli_sname);
      } else {
        printf("; %p in Unknown\n", (void*)ips[n - 1]);
      }
    }
  }
}



Assembly* Assembly::getExecutingAssembly() {
  int* ips[5];
  int real_size = mvm::jit::getBacktrace((void**)(void*)ips, 5);
  int n = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::jit::getCodeFromPointer(ips[n++]);
    if (code) {
      VMMethod* meth = (VMMethod*)code->getMetaInfo();
      if (meth) {
        return meth->classDef->assembly;
      }
    }
  }
  return 0;
}

Assembly* Assembly::getCallingAssembly() {
  int* ips[5];
  int real_size = mvm::jit::getBacktrace((void**)(void*)ips, 5);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::jit::getCodeFromPointer(ips[n++]);
    if (code) {
      VMMethod* meth = (VMMethod*)code->getMetaInfo();
      if (meth && i >= 1) {
        return meth->classDef->assembly;
      } else {
        ++i;
      }
    }
  }
  return 0;
}
