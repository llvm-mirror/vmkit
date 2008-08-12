//===--------- CommandLine.h - Parses the command line --------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <map>

#include <string.h>

#include "mvm/VirtualMachine.h"

#if defined(__APPLE__)
#define JNJVM_LIB "Jnjvm.dylib"
#define CLASSPATH_LIB "Classpath.dylib"
#define N3_LIB "N3.dylib"
#define PNET_LIB "Pnetlib.dylib"
#else
#define JNJVM_LIB "Jnjvm.so"
#define CLASSPATH_LIB "Classpath.so"
#define N3_LIB "N3.so"
#define PNET_LIB "Pnetlib.so"
#endif

typedef int (*boot_t)();
typedef mvm::VirtualMachine* (*create_vm_t)();

namespace mvm {


struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

class CommandLine {
public:
  char** argv;
  unsigned argc;
  unsigned argcMax;

  char* _yytext;
  unsigned _yylen;
  unsigned _yylenMax;
  char _yyChar;
  
  std::map<const char*, create_vm_t, ltstr> vmlets;

  CommandLine();
  
  void appendChar(char c);
  void appendString(char* str);

  void start();
  void executeInstr();

  void resetArgv();
  void resetString();

};

} // end namespace mvm

#endif // COMMAND_LINE_H
