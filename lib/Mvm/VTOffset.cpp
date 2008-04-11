//===-- VTOffset.cpp - Calculates compiler dependant VT offsets -----------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "MvmGC.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Thread.h"
#include "mvm/Sigsegv.h"
#include "mvm/VMLet.h"

class Toto : public mvm::Object {
public:
  
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const {
    printf("in print!\n");
  }
  virtual int hashCode() {
    printf("in hashcode!\n");
    return 1;
  }

  virtual void tracer(size_t sz) {
    printf("in tracer\n");
  }

  virtual void destroyer(size_t sz) {
    printf("in destroy!\n");
  }

  virtual ~Toto() {
    printf("in delete Toto!\n");
  }
};

static void clearSignals(void) {
  sys_signal(SIGINT,  SIG_DFL);
  sys_signal(SIGILL,  SIG_DFL);
#if !defined(WIN32)
  sys_signal(SIGIOT,  SIG_DFL);
  sys_signal(SIGBUS,  SIG_DFL);
#endif 
  sys_signal(SIGSEGV, SIG_DFL);
}


VirtualTable* Toto::VT = 0;
typedef void (*toto_t)(Toto* t);

int main(int argc, char **argv, char **envp) {
  int base;
  
  mvm::VMLet::initialise();
  mvm::Object::initialise(&base);
  
  initialiseVT();
  initialiseStatics();
  
  /*void* handle = sys_dlopen("libLisp.so", RTLD_LAZY | RTLD_GLOBAL);
  boot func = (boot)sys_dlsym(handle, "boot");
  func(argc, argv, envp);*/
  
  Toto t;
  Toto::VT =((void**)(void*)&t)[0];
  toto_t* ptr = (toto_t*)Toto::VT;
  printf("ptr[0] = %d, ptr[1]= %d, ptr[2] = %d ptr[3] = %d ptr[4] = %d ptr[5] = %d\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  ptr[0](&t);
  //ptr[1](&t); // This should be ~gc
  ptr[2](&t);
  ptr[3](&t);
  ptr[4](&t);
  ptr[5](&t);

  clearSignals();
  mvm::Thread::exit(0);
   
  return 0; 
}
