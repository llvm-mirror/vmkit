//===------------ Backtrace.cpp - Backtrace utilities ---------------------===//
//
//                               N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <cstdio>
#include <dlfcn.h>

#include "mvm/JIT.h"
#include "mvm/Object.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMThread.h"

using namespace n3;

// Do a backtrace until we know when we cross native -> CLI boundaries.
void CLIJit::printBacktrace() {
  int* ips[100];
  int real_size = mvm::Thread::get()->getBacktrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    Dl_info info;
    int res = dladdr(ips[n++], &info);
    if (res != 0) {
      fprintf(stderr, "; %p in %s\n",  (void*)ips[n - 1], info.dli_sname);
    } else {
      fprintf(stderr, "; %p in .Net method\n", (void*)ips[n - 1]);
    }
  }
}
