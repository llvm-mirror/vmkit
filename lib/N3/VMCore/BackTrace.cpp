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
    int *begIp = (int*)Collector::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          VMMethod* meth = (VMMethod*)m->definition();
          if (meth) 
            printf("; 0x%08x in %s\n", (uint32) ips[n - 1], meth->printString());
          else
            printf("; 0x%08x in %s\n", (uint32) ips[n - 1], ((llvm::Function*)glob)->getNameStr().c_str());
        } else VMThread::get()->vm->unknownError("in global variable?");
      } else printf("; 0x%08x in stub\n", (uint32) ips[n - 1]);
    } else {
      Dl_info info;
      int res = dladdr(begIp, &info);
      if (res != 0) {
        printf("; 0x%08x in %s\n", (uint32) ips[n - 1], info.dli_fname);
      } else {
        printf("; 0x%08x in Unknown\n", (uint32) ips[n - 1]);
      }
    }
  }
}



Assembly* Assembly::getExecutingAssembly() {
  int* ips[10];
  int real_size = backtrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    int *begIp = (int*)Collector::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          VMMethod* meth = (VMMethod*)m->definition();
          if (meth && meth->getVirtualTable() == VMMethod::VT) {
            return meth->classDef->assembly;
          } else {
            ++i;
          }
        } else {
          VMThread::get()->vm->unknownError("in global variable?");
        }
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
    int *begIp = (int*)Collector::begOf(ips[n++]);
    if (begIp) {
      const llvm::GlobalValue * glob = 
        mvm::jit::executionEngine->getGlobalValueAtAddress(begIp + 1);
      if (glob) {
        if (llvm::isa<llvm::Function>(glob)) {
          mvm::Code* c = (mvm::Code*)begIp;
          mvm::Method* m = c->method();
          VMMethod* meth = (VMMethod*)m->definition();
          if (meth && i >= 1 && meth->getVirtualTable() == VMMethod::VT) {
            return meth->classDef->assembly;
          } else {
            ++i;
          }
        } else {
          VMThread::get()->vm->unknownError("in global variable?");
        }
      }
    }
  }
  return 0;
}
