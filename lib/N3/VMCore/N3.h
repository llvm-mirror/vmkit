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
class N3ModuleProvider;
class StringMap;
class UTF8;
class UTF8Map;
class VMClass;
class VMClassArray;
class VMCommonClass;
class VMField;
class VMMethod;

class N3 : public VirtualMachine {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  VMObject*     asciizToStr(const char* asciiz);
  VMObject*     UTF8ToStr(const UTF8* utf8);
  Assembly*     constructAssembly(const UTF8* name);
  Assembly*     lookupAssembly(const UTF8* name);
  
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
  void mapInitialThread();
  void loadBootstrap();
  void waitForExit();
  
  static N3* bootstrapVM;

  static VMClass* pVoid;
  static VMClass* pBoolean;
  static VMClass* pChar;
  static VMClass* pSInt8;
  static VMClass* pUInt8;
  static VMClass* pSInt16;
  static VMClass* pUInt16;
  static VMClass* pSInt32;
  static VMClass* pUInt32;
  static VMClass* pSInt64;
  static VMClass* pUInt64;
  static VMClass* pFloat;
  static VMClass* pDouble;
  static VMClass* pIntPtr;
  static VMClass* pUIntPtr;
  static VMClass* pString;
  static VMClass* pObject;
  static VMClass* pValue;
  static VMClass* pEnum;
  static VMClass* pArray;
  static VMClass* pDelegate;
  static VMClass* pException;
  static VMClassArray* arrayChar;
  static VMClassArray* arrayString;
  static VMClassArray* arrayByte;
  static VMClassArray* arrayObject;
  static VMField* ctorBoolean;
  static VMField* ctorUInt8;
  static VMField* ctorSInt8;
  static VMField* ctorChar;
  static VMField* ctorSInt16;
  static VMField* ctorUInt16;
  static VMField* ctorSInt32;
  static VMField* ctorUInt32;
  static VMField* ctorSInt64;
  static VMField* ctorUInt64;
  static VMField* ctorIntPtr;
  static VMField* ctorUIntPtr;
  static VMField* ctorDouble;
  static VMField* ctorFloat;

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

  static VMMethod* ctorClrType;
  static VMClass* clrType;
  static VMField* typeClrType;
  
  static VMMethod* ctorAssemblyReflection;
  static VMClass* assemblyReflection;
  static VMClass* typedReference;
  static VMField* assemblyAssemblyReflection;
  static VMClass* propertyType;
  static VMClass* methodType;
  
  static VMMethod* ctorPropertyType;
  static VMMethod* ctorMethodType;
  static VMField* propertyPropertyType;
  static VMField* methodMethodType;

  static VMClass* resourceStreamType;
  static VMMethod* ctorResourceStreamType;
};

} // end namespace n3

#endif
