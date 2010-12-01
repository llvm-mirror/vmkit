//===------- CommandLine.cpp - Parses the command line --------------------===//
//
//                     The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#if 0

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "CommandLine.h"
#include "mvm/GC.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;

typedef struct thread_arg_t {
  int argc;
  char** argv;
  create_vm_t func;
} thread_arg_t;



CommandLine::CommandLine() {
  resetString();
  resetArgv();
}

void CommandLine::appendChar(char c) {
  assert(_yytext);
  _yytext[_yylen++]= c;
  if (_yylen == _yylenMax) {
    _yylenMax *= 2;
    _yytext= (char *)realloc(_yytext, _yylenMax);
  }
}

void CommandLine::appendString(char* str) {
  assert(argv);
  appendChar(0);
  argv[argc++] = str;
  if (argc == argcMax) {
    argcMax *= 2;
    argv = (char **)realloc(argv, argcMax * sizeof(char*));
  }
}

void CommandLine::resetString() {
  _yytext = (char *)malloc(_yylenMax= 32);
  _yylen = 0;
}

void CommandLine::resetArgv() {
  argv = (char **)malloc(sizeof(char*) * (argcMax= 10));
  argc = 0;
}


void CommandLine::start() {
  printf("> ");
  _yyChar = getc(stdin);
  
  while (true) {
    switch(_yyChar) {
      case ' ' : 
        do { _yyChar = getc(stdin); } while (_yyChar == ' ');
        if (_yylen != 0) {
          appendString(_yytext);
          resetString();
        }
        break;
      
      case '\n' :
        if (_yylen != 0) {
          appendString(_yytext);
          resetString();
        }
        if (argc > 1) {
          executeInstr();
          resetArgv();
          printf("> ");
        }
        _yyChar = getc(stdin);
        break;

      case EOF :
        printf("\n");
        return;

      default :
        appendChar(_yyChar);
        _yyChar = getc(stdin);
    }
  } 
}

#if 0
extern "C" int startApp(thread_arg_t* arg) {
  int argc = arg->argc;
  char** argv = arg->argv;
  create_vm_t func = arg->func;
  free(arg);
#ifndef MULTIPLE_GC
  Collector::inject_my_thread(&argc);
  VirtualMachine* VM = func();
  VM->runApplication(argc, argv);
  Collector::remove_my_thread();
  Collector::collect();
#else
  Collector* GC = Collector::allocate();
  GC->inject_my_thread(&argc);
  func(argc, argv);
  GC->remove_my_thread();
  GC->collect();
#endif
  return 0;
}
#endif

void CommandLine::executeInstr() {
  if (!strcmp(argv[0], "load")) {
#if defined(__APPLE__)
    char* buf = (char*)alloca(sizeof(argv[1]) + 7);
    sprintf(buf, "%s.dylib", argv[1]);
#else
    char* buf = (char*)alloca(sizeof(argv[1]) + 4);
    sprintf(buf, "%s.so", argv[1]);
#endif
    void* handle = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
    if (handle == 0) {
      fprintf(stderr, "\t Unable to load %s\n", argv[1]);
      printf("\t error = %s\n", dlerror());
      return;
    }
    
    boot_t func = (boot_t)(intptr_t)dlsym(handle, "initialiseVirtualMachine");
    
    if (func == 0) {
      fprintf(stderr, "\t Unable to find %s boot method\n", argv[1]);
      dlclose(handle);
      return;
    }
    func();
    
    create_vm_t vmlet = (create_vm_t)(intptr_t)dlsym(handle, "createVirtualMachine");

    vmlets[argv[1]] = vmlet;

  } else {
    create_vm_t func = vmlets[argv[0]];
    mvm::Object* CU = compilers[argv[0]];
    if (!func) {
      fprintf(stderr, "\t Unknown vm %s\n", argv[0]);
    } else {
#if 0
      thread_arg_t* thread_arg = (thread_arg_t*)malloc(sizeof (thread_arg_t));
      thread_arg->argc = argc;
      thread_arg->argv = argv;
      thread_arg->func = func;
      int tid = 0;
      Thread::start(&tid, (int (*)(void *))startApp, thread_arg);
#else
      VirtualMachine* VM = func(CU);
      VM->runApplication(argc, argv);
#endif
    }
  }
}

#endif
