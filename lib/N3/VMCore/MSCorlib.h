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
};

} // end namespace n3

#endif // N3_MSCORLIB_H
