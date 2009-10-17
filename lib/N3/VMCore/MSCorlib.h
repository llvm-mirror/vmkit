//===------------- MSCorlib.h - The MSCorlib interface --------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_MSCORLIB_H
#define N3_MSCORLIB_H

#include "N3MetaType.h"

namespace n3 {

class N3;
class VMClass;
class VMClassArray;
class VMField;
class VMMethod;

class MSCorlib {
public:
  static void initialise(N3* vm);
  static void loadStringClass(N3* vm);
  static void loadBootstrap(N3* vm);
  
  static const char* libsPath;

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

#define DEF_TYPE(name, type)        \
	static VMClass* p##name;

#define DEF_ARRAY_AND_TYPE(name, type) \
	DEF_TYPE(name, type) \
  static VMClassArray* array##name; \
	static VMField* ctor##name;

	ON_TYPES(DEF_ARRAY_AND_TYPE, _F_NT)
	ON_STRING(DEF_ARRAY_AND_TYPE, _F_NT)
	ON_VOID(DEF_TYPE, _F_NT)

#undef DEF_ARRAY_AND_TYPE
#undef DEF_TYPE
 
  static VMClass* pValue;
  static VMClass* pEnum;
  static VMClass* pArray;
  static VMClass* pDelegate;
  static VMClass* pException;
};

} // end namespace n3

#endif // N3_MSCORLIB_H
