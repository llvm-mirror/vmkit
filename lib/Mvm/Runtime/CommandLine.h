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

namespace mvm {

typedef int (*vmlet_main_t)(int argc, char** argv);

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
  
  std::map<const char*, vmlet_main_t, ltstr> vmlets;

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
