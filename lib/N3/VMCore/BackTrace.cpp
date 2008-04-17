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

#include "llvm/Function.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "mvm/JIT.h"
#include "mvm/Method.h"
#include "mvm/Object.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "CLIString.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMThread.h"

#include <execinfo.h>

using namespace n3;

void CLIJit::printBacktrace() {
  int* ips[100];
  int real_size = backtrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth && meth->getVirtualTable() == VMMethod::VT) {
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



Assembly* Assembly::getExecutingAssembly() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 10);
  int n = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth && meth->getVirtualTable() == VMMethod::VT) {
        return ((VMMethod*)meth)->classDef->assembly;
      }
    }
  }
  return 0;
}

Assembly* Assembly::getCallingAssembly() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    mvm::Code* code = mvm::Code::getCodeFromPointer(ips[n++]);
    if (code) {
      mvm::Method* m = code->method();
      mvm::Object* meth = m->definition();
      if (meth && i >= 1 && meth->getVirtualTable() == VMMethod::VT) {
        return ((VMMethod*)meth)->classDef->assembly;
      } else {
        ++i;
      }
    }
  }
  return 0;
}
