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

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"

#include "mvm/Object.h"

#include "types.h"

#include "JavaArray.h"

namespace jnjvm {

class CommonClass;
class JavaJIT;
class JavaObject;
class Jnjvm;
class UTF8;

class AssessorDesc : public mvm::Object {
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

  const char* asciizName;
  CommonClass* classType;
  const llvm::Type* llvmType;
  const llvm::Type* llvmTypePtr;
  llvm::Constant* llvmNullConstant;
  const UTF8* assocClassName;
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
  
  static AssessorDesc* allocate(bool dt, char bid, uint32 nb, uint32 nw,
                                const char* name, Jnjvm* vm,
                                const llvm::Type* type, const char* assocName,
                                arrayCtor_t ctor);

  static void initialise(Jnjvm* vm);
  

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

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

  static void arrayType(Jnjvm *vm, JavaObject* loader, unsigned int t,
                        ClassArray*& cl, AssessorDesc*& ass, 
                        llvm::Function*& ctr);
  
  static AssessorDesc* byteIdToPrimitive(const char id);
  static AssessorDesc* classToPrimitive(CommonClass* cl);
  static AssessorDesc* bogusClassToPrimitive(CommonClass* cl);

};


class Typedef : public mvm::Object {
public:
  static VirtualTable *VT;
  const UTF8* keyName;
  const UTF8* pseudoAssocClassName;
  const AssessorDesc* funcs;
  Jnjvm* isolate;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  
  CommonClass* assocClass(JavaObject* loader);
  void typePrint(mvm::PrintBuffer* buf);
  static void humanPrintArgs(const std::vector<Typedef*>*, mvm::PrintBuffer* buf);
  static Typedef* typeDup(const UTF8* name, Jnjvm* vm);
  void tPrintBuf(mvm::PrintBuffer* buf) const;

};

class Signdef : public Typedef {
public:
  static VirtualTable *VT;
  std::vector<Typedef*> args;
  Typedef* ret;
  const llvm::FunctionType* staticType;
  const llvm::FunctionType* virtualType;
  const llvm::FunctionType* nativeType;
  const llvm::FunctionType* virtualBufType;
  const llvm::FunctionType* staticBufType;
  const llvm::PointerType* staticTypePtr;
  const llvm::PointerType* virtualTypePtr;
  const llvm::PointerType* nativeTypePtr;
  mvm::Code* _staticCallBuf;
  mvm::Code* _virtualCallBuf;
  mvm::Code* _staticCallAP;
  mvm::Code* _virtualCallAP;
  uint32 nbIn;
  
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

  static const llvm::FunctionType* createVirtualType(
                        const std::vector<Typedef*>*, Typedef*);
  
  static const llvm::FunctionType* createStaticType(
                        const std::vector<Typedef*>*, Typedef*);
  
  static const llvm::FunctionType* createNativeType(
                        const std::vector<Typedef*>*, Typedef*);
  
  llvm::Function* createFunctionCallBuf(bool virt);
  llvm::Function* createFunctionCallAP(bool virt);
  void createFuncPtrsCalls();

  

  void printWithSign(CommonClass* cl, const UTF8* name, mvm::PrintBuffer* buf);
  unsigned int nbInWithThis(unsigned int flag);
  static Signdef* signDup(const UTF8* name, Jnjvm* vm);
  
  void* staticCallBuf();
  void* virtualCallBuf();
  void* staticCallAP();
  void* virtualCallAP();

};

} // end namespace jnjvm

#endif
