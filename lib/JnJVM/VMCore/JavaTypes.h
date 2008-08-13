//===--------------- JavaTypes.h - Java primitives ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_TYPES_H
#define JNJVM_JAVA_TYPES_H

#include "types.h"

#include "mvm/JIT.h"
#include "mvm/Object.h"

namespace jnjvm {

class ClassArray;
class CommonClass;
class JavaArray;
class JavaJIT;
class JavaObject;
class Jnjvm;
class UTF8;

#define VOID_ID 0
#define BOOL_ID 1
#define BYTE_ID 2
#define CHAR_ID 3
#define SHORT_ID 4
#define INT_ID 5
#define FLOAT_ID 6
#define LONG_ID 7
#define DOUBLE_ID 8
#define ARRAY_ID 9
#define OBJECT_ID 10
#define NUM_ASSESSORS 11

typedef JavaArray* (*arrayCtor_t)(uint32 len, CommonClass* cl, Jnjvm* vm);

class AssessorDesc {
public:
  static VirtualTable *VT;
  static const char I_TAB;
  static const char I_END_REF;
  static const char I_PARG;
  static const char I_PARD;
  static const char I_BYTE;
  static const char I_CHAR;
  static const char I_DOUBLE;
  static const char I_FLOAT;
  static const char I_INT;
  static const char I_LONG;
  static const char I_REF;
  static const char I_SHORT;
  static const char I_VOID;
  static const char I_BOOL;
  static const char I_SEP;
  
  bool doTrace;
  char byteId;
  uint32 nbb;
  uint32 nbw;
  uint8 numId;

  const char* asciizName;
  CommonClass* classType;
  const UTF8* assocClassName;
  const UTF8* UTF8Name;
  ClassArray* arrayClass;
  arrayCtor_t arrayCtor;

  static AssessorDesc* dParg;
  static AssessorDesc* dPard;
  static AssessorDesc* dVoid;
  static AssessorDesc* dBool;
  static AssessorDesc* dByte;
  static AssessorDesc* dChar;
  static AssessorDesc* dShort;
  static AssessorDesc* dInt;
  static AssessorDesc* dFloat;
  static AssessorDesc* dLong;
  static AssessorDesc* dDouble;
  static AssessorDesc* dTab;
  static AssessorDesc* dRef;
  
  AssessorDesc(bool dt, char bid, uint32 nb, uint32 nw,
               const char* name,
               Jnjvm* vm, uint8 nid,
               const char* assocName, ClassArray* cl,
               arrayCtor_t ctor);

  static void initialise(Jnjvm* vm);
  

  const char* printString() const;

  static void analyseIntern(const UTF8* name, uint32 pos,
                            uint32 meth, AssessorDesc*& ass,
                            uint32& ret);

  static const UTF8* constructArrayName(Jnjvm *vm, AssessorDesc* ass,
                                        uint32 steps, const UTF8* className);
  
  static void introspectArrayName(Jnjvm *vm, const UTF8* utf8, uint32 start,
                                  AssessorDesc*& ass, const UTF8*& res);
  
  static void introspectArray(Jnjvm *vm, JavaObject* loader, const UTF8* utf8,
                              uint32 start, AssessorDesc*& ass,
                              CommonClass*& res);

  static AssessorDesc* arrayType(unsigned int t);
  
  static AssessorDesc* byteIdToPrimitive(const char id);
  static AssessorDesc* classToPrimitive(CommonClass* cl);

};


class Typedef {
public:
  const UTF8* keyName;
  const UTF8* pseudoAssocClassName;
  const AssessorDesc* funcs;
  Jnjvm* isolate;

  const char* printString() const;
  
  CommonClass* assocClass(JavaObject* loader);
  void typePrint(mvm::PrintBuffer* buf);
  static void humanPrintArgs(const std::vector<Typedef*>*,
                             mvm::PrintBuffer* buf);
  static Typedef* typeDup(const UTF8* name, Jnjvm* vm);
  void tPrintBuf(mvm::PrintBuffer* buf) const;

};

class Signdef {
private:
  intptr_t _staticCallBuf;
  intptr_t _virtualCallBuf;
  intptr_t _staticCallAP;
  intptr_t _virtualCallAP;
  
  intptr_t staticCallBuf();
  intptr_t virtualCallBuf();
  intptr_t staticCallAP();
  intptr_t virtualCallAP();

public:
  std::vector<Typedef*> args;
  Typedef* ret;
  Jnjvm* isolate;
  const UTF8* keyName;
  
  const char* printString() const;

  void printWithSign(CommonClass* cl, const UTF8* name, mvm::PrintBuffer* buf);
  static Signdef* signDup(const UTF8* name, Jnjvm* vm);
  
  
  intptr_t getStaticCallBuf() {
    if(!_staticCallBuf) return staticCallBuf();
    return _staticCallBuf;
  }

  intptr_t getVirtualCallBuf() {
    if(!_virtualCallBuf) return virtualCallBuf();
    return _virtualCallBuf;
  }
  
  intptr_t getStaticCallAP() {
    if (!_staticCallAP) return staticCallAP();
    return _staticCallAP;
  }

  intptr_t getVirtualCallAP() {
    if (!_virtualCallAP) return virtualCallAP();
    return _virtualCallAP;
  }
  
  void setStaticCallBuf(intptr_t code) {
    _staticCallBuf = code;
  }

  void setVirtualCallBuf(intptr_t code) {
    _virtualCallBuf = code;
  }
  
  void setStaticCallAP(intptr_t code) {
    _staticCallAP = code;
  }

  void setVirtualCallAP(intptr_t code) {
    _virtualCallAP = code;
  }

   
  mvm::JITInfo* JInfo;
  template<typename Ty> 
  Ty *getInfo() {
    if (!JInfo) {
      JInfo = new Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }
    
};


} // end namespace jnjvm

#endif
