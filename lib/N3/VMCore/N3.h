//===---------------- N3.h - The N3 virtual machine -----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef N3_N3_H
#define N3_N3_H

#include "types.h"

#include "VirtualMachine.h"

namespace n3 {

class ArrayObject;
class ArrayUInt8;
class Assembly;
class AssemblyMap;
class FunctionMap;
class N3;
class N3ModuleProvider;
class StringMap;
class UTF8;
class UTF8Map;
class VMClass;
class VMClassArray;
class VMCommonClass;
class VMField;
class VMMethod;

class ClArgumentsInfo {
public:
  int argc;
  char** argv;
  uint32 appArgumentsPos;
  char* assembly;

  void readArgs(int argc, char** argv, N3 *vm);

  void printInformation();
  void nyi();
  void printVersion();
};


class N3 : public VirtualMachine {
public:
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

	N3(mvm::BumpPtrAllocator &allocator) : VirtualMachine(allocator) {}

  VMObject*     asciizToStr(const char* asciiz);
  VMObject*     UTF8ToStr(const UTF8* utf8);
  Assembly*     constructAssembly(const UTF8* name);
  Assembly*     lookupAssembly(const UTF8* name);
  
  ClArgumentsInfo argumentsInfo;
  const char* name;
  StringMap * hashStr;
  AssemblyMap* loadedAssemblies;
  std::vector<const char*> assemblyPath;
  Assembly* coreAssembly;


  static N3* allocateBootstrap();
  static N3* allocate(const char* name, N3* parent);
  
  ArrayUInt8* openAssembly(const UTF8* name, const char* extension);
 
  Assembly* loadAssembly(const UTF8* name, const char* extension);
  void executeAssembly(const char* name, ArrayObject* args);
  void runMain(int argc, char** argv);
  virtual void waitForExit();
  static void mainCLIStart(VMThread* th);
  
  static N3* bootstrapVM;
 
  static const UTF8* clinitName;
  static const UTF8* ctorName;
  static const UTF8* invokeName;
  static const UTF8* math;
  static const UTF8* system;
  static const UTF8* sqrt;
  static const UTF8* sin;
  static const UTF8* cos;
  static const UTF8* exp;
  static const UTF8* log;
  static const UTF8* floor;
  static const UTF8* log10;
  static const UTF8* isNan;
  static const UTF8* pow;
  static const UTF8* floatName;
  static const UTF8* doubleName;
  static const UTF8* testInfinity;
  
};

} // end namespace n3

#endif
