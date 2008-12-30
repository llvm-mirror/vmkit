//===---------- JavaBacktrace.cpp - Backtrace utilities -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <dlfcn.h>

#include "mvm/JIT.h"
#include "mvm/Object.h"

#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"

using namespace jnjvm;

#if 0
JavaMethod* JavaJIT::IPToJavaMethod(void* begIp) {
  const llvm::Function* F = mvm::MvmModule::getCodeFromPointer(begIp);
  if (F) {
    JavaMethod* meth = LLVMMethodInfo::get(F);
    return meth;
  }
  return 0;
}

void JavaJIT::printBacktrace() {
  int* ips[100];
  int real_size = mvm::MvmModule::getBacktrace((void**)(void*)ips, 100);
  int n = 0;
  while (n < real_size) {
    JavaMethod* meth = IPToJavaMethod(ips[n++]);
    if (meth) {
      fprintf(stderr, "; %p in %s\n",  (void*)ips[n - 1], meth->printString());
    } else {
      Dl_info info;
      int res = dladdr(ips[n++], &info);
      if (res != 0) {
        fprintf(stderr, "; %p in %s\n",  (void*)ips[n - 1], info.dli_sname);
      } else {
        fprintf(stderr, "; %p in Unknown\n", (void*)ips[n - 1]);
      }
    }
  }
}




#ifndef ISOLATE_SHARING
UserClass* JavaJIT::getCallingClass() {
  int* ips[10];
  int real_size = mvm::MvmModule::getBacktrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    JavaMethod* meth = IPToJavaMethod(ips[n++]);
    if (meth) {
      if (i == 1) {
        return meth->classDef;
      } else {
        ++i;
      }
    }
  }
  return 0;
}

UserClass* JavaJIT::getCallingClassWalker() {
  int* ips[10];
  int real_size = mvm::MvmModule::getBacktrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    JavaMethod* meth = IPToJavaMethod(ips[n++]);
    if (meth) {
      if (i == 1) {
        return meth->classDef;
      } else {
        ++i;
      }
    }
  }
  return 0;
}
#else

UserClass* JavaJIT::getCallingClass() {
  Class* res = 0;

  int* ips[10];
  int real_size = mvm::MvmModule::getBacktrace((void**)(void*)ips, 10);
  int n = 0;
  int i = 0;
  while (n < real_size) {
    JavaMethod* meth = IPToJavaMethod(ips[n++]);
    if (meth) {
      if (i == 1) {
        res = meth->classDef;
        break;
      } else {
        ++i;
      }
    }
  }

  if (!res) return 0;

  unsigned int* top;
  register unsigned int  **cur = &top;
  register unsigned int  **max = (unsigned int**)mvm::Thread::get()->baseSP;
    
  for(; cur<max; cur++) {
    void* obj = (void*)(*cur);
    obj = Collector::begOf(obj);
    if (obj && ((mvm::Object*)obj)->getVirtualTable() == UserConstantPool::VT) {
      UserConstantPool* ctp = (UserConstantPool*)obj;
      UserClass* cl = ctp->getClass();
      if (cl->classDef == res) {
        return cl;
      }
    }
  }
  return 0;
}

UserClass* JavaJIT::getCallingClassWalker() {
  return getCallingClass();
}
#endif

JavaObject* JavaJIT::getCallingClassLoader() {
  UserClass* cl = getCallingClassWalker();
  if (!cl) return 0;
  else return cl->classLoader->getJavaClassLoader();
}

#endif
