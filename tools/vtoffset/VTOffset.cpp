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

class Toto : public mvm::Object {
public:
  
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const {
    printf("in print!\n");
  }
  virtual intptr_t hashCode() {
    printf("in hashcode!\n");
    return 1;
  }

  virtual void tracer() {
    printf("in tracer\n");
  }

  virtual void destroyer(size_t sz) {
    printf("in destroy!\n");
  }

  ~Toto() {
    printf("in delete Toto!\n");
  }
};

class Tata : public Toto {
  public:
  static VirtualTable* VT;
  ~Tata() {
    printf("in delete Tata!\n");
  }
};

VirtualTable* Toto::VT = 0;
typedef void (*toto_t)(Toto* t);

VirtualTable* Tata::VT = 0;
typedef void (*tata_t)(Tata* t);

int main(int argc, char **argv, char **envp) {
  int base;
  
  mvm::Object::initialise();
  
  /*void* handle = sys_dlopen("libLisp.so", RTLD_LAZY | RTLD_GLOBAL);
  boot func = (boot)sys_dlsym(handle, "boot");
  func(argc, argv, envp);*/
  { 
  Toto t;
  Toto::VT =((void**)(void*)&t)[0];
  toto_t* ptr = (toto_t*)Toto::VT;
  printf("ptr[0] = %d, ptr[1]= %d, ptr[2] = %d ptr[3] = %d ptr[4] = %d ptr[5] = %d\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  printf("0 = \n");
  ptr[0](&t);
  //ptr[1](&t); // operator delete
  printf("2 = \n");
  ptr[2](&t);
  printf("3 = \n");
  ptr[3](&t);
  printf("4 = \n");
  ptr[4](&t);
  printf("5 = \n");
  ptr[5](&t);
}
{
  Tata t;
  Tata::VT =((void**)(void*)&t)[0];
  tata_t* ptr = (tata_t*)Tata::VT;
  printf("ptr[0] = %d, ptr[1]= %d, ptr[2] = %d ptr[3] = %d ptr[4] = %d ptr[5] = %d\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  ptr[0](&t);
  printf("End\n");
  //ptr[1](&t); // operator delete
  ptr[2](&t);
  ptr[3](&t);
  ptr[4](&t);
  ptr[5](&t);
}
Tata* t = gc_new(Tata)();
  mvm::Thread::exit(0);
   
  return 0; 
}
