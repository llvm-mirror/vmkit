//===--- JavaConstantPool.h - Java constant pool definition ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CONSTANT_POOL_H
#define JNJVM_JAVA_CONSTANT_POOL_H

#include "mvm/Object.h"

#include "types.h"

namespace jnjvm {

class Class;
class CommonClass;
class Jnjvm;
class JavaField;
class JavaMethod;
class JavaString;
class Reader;
class Signdef;
class Typedef;
class UTF8;

typedef uint32 (*ctpReader)(Class*, uint32, uint32, Reader&, sint32*, void**, uint8*);

class JavaCtpInfo {
public:
  Class*  classDef;
  void**  ctpRes;
  sint32* ctpDef;
  uint8*  ctpType;
  uint32 ctpSize;
  
  static const uint32 ConstantUTF8;
  static const uint32 ConstantInteger;
  static const uint32 ConstantFloat;
  static const uint32 ConstantLong;
  static const uint32 ConstantDouble;
  static const uint32 ConstantClass;
  static const uint32 ConstantString;
  static const uint32 ConstantFieldref;
  static const uint32 ConstantMethodref;
  static const uint32 ConstantInterfaceMethodref;
  static const uint32 ConstantNameAndType;
  
  static ctpReader funcsReader[16];

  static uint32 CtpReaderClass(Class*, uint32 type, uint32 e, Reader& reader,
                            sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderInteger(Class*, uint32 type, uint32 e, Reader& reader,
                              sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderFloat(Class*, uint32 type, uint32 e, Reader& reader,
                            sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderUTF8(Class*, uint32 type, uint32 e, Reader& reader,
                           sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderNameAndType(Class*, uint32 type, uint32 e, Reader& reader,
                                  sint32* ctpDef, void** ctpRes,
                                  uint8* ctpType);
  
  static uint32 CtpReaderFieldref(Class*, uint32 type, uint32 e, Reader& reader,
                               sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderString(Class*, uint32 type, uint32 e, Reader& reader,
                             sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderMethodref(Class*, uint32 type, uint32 e, Reader& reader,
                                sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderInterfaceMethodref(Class*, uint32 type, uint32 e,
                                         Reader& reader, sint32* ctpDef,
                                         void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderLong(Class*, uint32 type, uint32 e, Reader& reader,
                           sint32* ctpDef, void** ctpRes, uint8* ctpType);
  
  static uint32 CtpReaderDouble(Class*, uint32 type, uint32 e, Reader& reader,
                             sint32* ctpDef, void** ctpRes, uint8* ctpType);

  static void read(Class* cl, Reader& reader);

  bool isAStaticCall(uint32 index) {
    return (ctpType[index] & 0x80) != 0;    
  }

  void markAsStaticCall(uint32 index) {
    ctpType[index] |= 0x80;
  }

  uint8 typeAt(uint32 index) {
    return ctpType[index] & 0x7F;    
  }

  const UTF8* UTF8At(uint32 entry);
  float FloatAt(uint32 entry);
  sint32 IntegerAt(uint32 entry);
  sint64 LongAt(uint32 entry);
  double DoubleAt(uint32 entry);

  CommonClass* isLoadedClassOrClassName(uint32 index);
  const UTF8* resolveClassName(uint32 index);
  CommonClass* loadClass(uint32 index);
  void checkInfoOfClass(uint32 index);
  Typedef* resolveNameAndType(uint32 index);
  Signdef* resolveNameAndSign(uint32 index);
  Typedef* infoOfField(uint32 index);
  Signdef* infoOfInterfaceOrVirtualMethod(uint32 index);
  void* infoOfStaticOrSpecialMethod(uint32 index,
                                    uint32 access,
                                    Signdef*& sign,
                                    JavaMethod*& meth);

  void nameOfStaticOrSpecialMethod(uint32 index, const UTF8*& cl, 
                                   const UTF8*& name, Signdef*& sign);
  
  uint32 getClassIndexFromMethod(uint32 index);

  CommonClass* getMethodClassIfLoaded(uint32 index);
  void resolveInterfaceOrMethod(uint32 index, CommonClass*& cl,
                                const UTF8*& utf8, Signdef*& sign);
  void infoOfMethod(uint32 index, uint32 access, CommonClass*& cl,
                    JavaMethod*& meth); 
  void resolveField(uint32 index, CommonClass*& cl, const UTF8*& utf8,
                    Typedef*& sign);
  
  JavaString* resolveString(const UTF8* utf8, uint16 index);
  
  JavaField* lookupField(uint32 index, bool stat);


};

} // end namespace jnjvm

#endif
