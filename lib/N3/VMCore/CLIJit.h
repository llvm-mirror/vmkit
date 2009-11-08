//===------------- CLIJit.h - CLI just in time compiler -------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_CLIJit_H
#define N3_CLIJit_H

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "llvm/BasicBlock.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/PassManager.h"
#include "llvm/Type.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Target/TargetData.h"

#include "types.h"

namespace n3 {

class CacheNode;
class N3;
class N3ModuleProvider;
class VMClass;
class VMClassArray;
class VMCommonClass;
class VMMethod;
class VMObject;
class VMGenericClass;
class VMGenericMethod;
class N3VirtualTable;

class ExceptionBlockDesc : public mvm::PermanentObject {
public:
	uint32 tryOffset;
	uint32 tryLength;
	uint32 handlerOffset;
	uint32 handlerLength;
	VMCommonClass* catchClass;
	llvm::BasicBlock* test;
	llvm::BasicBlock* realTest;
	llvm::BasicBlock* handler;

	virtual void print(mvm::PrintBuffer* buf) const;
};

class Opinfo {
public:
  llvm::BasicBlock* newBlock;
  llvm::BasicBlock* exceptionBlock;
  bool reqSuppl;
  llvm::Value* exception;
  
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Opinfo");
  }
};


class CLIJit : public mvm::PermanentObject {
public:
	mvm::BumpPtrAllocator &allocator;

	CLIJit(mvm::BumpPtrAllocator &a) : allocator(a) {}

  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("CLIJit");
  }
  
  static const char* OpcodeNames[0xE1];
  static const char* OpcodeNamesFE[0x23];

  static N3VirtualTable* makeVT(VMClass* cl, bool isStatic);
  static N3VirtualTable* makeArrayVT(VMClassArray* cl);
  
  static void printExecution(char*, n3::VMMethod*);
  void compileOpcodes(uint8*, uint32, VMGenericClass* genClass, VMGenericMethod* genMethod);
  void exploreOpcodes(uint8*, uint32);
  
  llvm::Function* llvmFunction;
  VMMethod* compilingMethod;
  VMClass* compilingClass;
  mvm::MvmModule* module;

  std::vector<llvm::Value*> arguments;
  std::vector<llvm::Value*> locals;
  
  // end function
  llvm::BasicBlock* endBlock;
  llvm::PHINode* endNode;

  // block manipulation
  llvm::BasicBlock* currentBlock;
  llvm::BasicBlock* createBasicBlock(const char* name = "");
  void setCurrentBlock(llvm::BasicBlock* block);

  // branches
  void branch(llvm::Value* test, llvm::BasicBlock* ifTrue,
              llvm::BasicBlock* ifFalse, llvm::BasicBlock* insert);
  void branch(llvm::BasicBlock* where, llvm::BasicBlock* insert);
   
  // stack manipulation
  std::vector<llvm::Value*> stack;
  void push(llvm::Value*);
  llvm::Value* pop();
  llvm::Value* top();
  
  // exceptions
  llvm::BasicBlock*                endExceptionBlock;
  llvm::BasicBlock*                currentExceptionBlock;
  llvm::BasicBlock*                unifiedUnreachable;
  std::vector<ExceptionBlockDesc*> exceptions;
  std::vector<ExceptionBlockDesc*> finallyHandlers;

  uint32 readExceptionTable(uint32 offset, bool fat, VMGenericClass* genClass, VMGenericMethod* genMethod);
  std::vector<llvm::BasicBlock*>   leaves; 
  llvm::Value* supplLocal;

  // calls
  void invoke(uint32 value, VMGenericClass* genClass, VMGenericMethod* genMethod);
  void invokeInterfaceOrVirtual(uint32 value, VMGenericClass* genClass, VMGenericMethod* genMethod);
  void invokeNew(uint32 value, VMGenericClass* genClass, VMGenericMethod* genMethod);
  llvm::Value* getVirtualField(uint32 value, VMGenericClass* genClass, VMGenericMethod* genMethod);
  llvm::Value* getStaticField(uint32 value, VMGenericClass* genClass, VMGenericMethod* genMethod);
  void setVirtualField(uint32 value, bool isVolatile, VMGenericClass* genClass, VMGenericMethod* genMethod);
  void setStaticField(uint32 value, bool isVolatile, VMGenericClass* genClass, VMGenericMethod* genMethod);

  void JITVerifyNull(llvm::Value* obj);
  
  // array manipulation
  llvm::Value* verifyAndComputePtr(llvm::Value* obj, llvm::Value* index,
                                   const llvm::Type* arrayType,
                                   bool verif = true);
  llvm::Value* arraySize(llvm::Value* obj);

  Opinfo* opcodeInfos;

  static llvm::Function* printExecutionLLVM;
  static llvm::Function* indexOutOfBoundsExceptionLLVM;
  static llvm::Function* nullPointerExceptionLLVM;
  static llvm::Function* initialiseClassLLVM;
  static llvm::Function* throwExceptionLLVM;
  static llvm::Function* clearExceptionLLVM;
  static llvm::Function* compareExceptionLLVM;
  static llvm::Function* classCastExceptionLLVM;
  static llvm::Function* newStringLLVM;
  static llvm::Function* arrayLengthLLVM;

  static llvm::Function* compile(VMClass* cl, VMMethod* meth);

#ifdef WITH_TRACER
  static llvm::Function* markAndTraceLLVM;
  static const llvm::FunctionType* markAndTraceLLVMType;
#endif
  static llvm::Function* vmObjectTracerLLVM;
  static llvm::Function* arrayConsLLVM;
  static llvm::Function* arrayMultiConsLLVM;
  static llvm::Function* objConsLLVM;
  static llvm::Function* objInitLLVM;
  static llvm::Function* instanceOfLLVM;
  static llvm::Function* getCppExceptionLLVM;
  static llvm::Function* getCLIExceptionLLVM;
  
  static void initialise();
  static void initialiseAppDomain(N3* vm);
  static void initialiseBootstrapVM(N3* vm);

  llvm::Function* compileNative(VMGenericMethod* genMethod);
  llvm::Function* compileFatOrTiny(VMGenericClass* genClass, VMGenericMethod* genMethod);
  llvm::Function* compileIntern();

  llvm::Function* createDelegate();
  llvm::Function* invokeDelegate();

  llvm::Value* invoke(llvm::Value *F, std::vector<llvm::Value*> args,
                      const char* Name,
                      llvm::BasicBlock *InsertAtEnd, bool structReturn);
  // Alternate CallInst ctors w/ two actuals, w/ one actual and no
  // actuals, respectively.
  llvm::Value* invoke(llvm::Value *F, llvm::Value *Actual1,
                      llvm::Value *Actual2, const char* Name,
                      llvm::BasicBlock *InsertAtEnd, bool structReturn);
  llvm::Value* invoke(llvm::Value *F, llvm::Value *Actual1,
                      const char* Name, llvm::BasicBlock *InsertAtEnd,
                      bool structReturn);
  llvm::Value* invoke(llvm::Value *F, const char* Name,
                      llvm::BasicBlock *InsertAtEnd, bool structReturn);
  
  void makeArgs(const llvm::FunctionType* type, 
                std::vector<llvm::Value*>& Args, bool structReturn);
  llvm::Value* changeType(llvm::Value* val, const llvm::Type* type);
  
  static llvm::Function* virtualLookupLLVM;

  llvm::Instruction* lowerMathOps(VMMethod* meth, 
                                  std::vector<llvm::Value*>& args);

  static llvm::Constant* constantVMObjectNull;

  llvm::Instruction* inlineCompile(llvm::Function* parentFunction, 
                                   llvm::BasicBlock*& curBB,
                                   llvm::BasicBlock* endExBlock,
                                   std::vector<llvm::Value*>& args, VMGenericClass* genClass, VMGenericMethod* genMethod);

  std::map<VMMethod*, bool> inlineMethods;

  llvm::Instruction* invokeInline(VMMethod* meth, 
                                  std::vector<llvm::Value*>& args, VMGenericClass* genClass, VMGenericMethod* genMethod);

  static VMMethod* getMethod(llvm::Function* F);
};

enum Opcode {
  NOP = 0x00,
  BREAK = 0x01,
  LDARG_0 = 0x02,
  LDARG_1 = 0x03,
  LDARG_2 = 0x04,
  LDARG_3 = 0x05,
  LDLOC_0 = 0x06,
  LDLOC_1 = 0x07,
  LDLOC_2 = 0x08,
  LDLOC_3 = 0x09,
  STLOC_0 = 0x0A,
  STLOC_1 = 0x0B,
  STLOC_2 = 0x0C,
  STLOC_3 = 0x0D,
  LDARG_S = 0x0E,
  LDARGA_S = 0x0F,
  STARG_S = 0x10,
  LDLOC_S = 0x11,
  LDLOCA_S = 0x12,
  STLOC_S = 0x13,
  LDNULL = 0x14,
  LDC_I4_M1 = 0x15,
  LDC_I4_0 = 0x16,
  LDC_I4_1 = 0x17,
  LDC_I4_2 = 0x18,
  LDC_I4_3 = 0x19,
  LDC_I4_4 = 0x1A,
  LDC_I4_5 = 0x1B,
  LDC_I4_6 = 0x1C,
  LDC_I4_7 = 0x1D,
  LDC_I4_8 = 0x1E,
  LDC_I4_S = 0x1F,
  LDC_I4 = 0x20,
  LDC_I8 = 0x21,
  LDC_R4 = 0x22,
  LDC_R8 = 0x23,
  UNUSED99 = 0x24,
  DUP = 0x25,
  POP = 0x26,
  JMP = 0x27,
  CALL = 0x28,
  CALLI = 0x29,
  RET = 0x2A,
  BR_S = 0x2B,
  BRFALSE_S = 0x2C,
  BRTRUE_S = 0x2D,
  BEQ_S = 0x2E,
  BGE_S = 0x2F,
  BGT_S = 0x30,
  BLE_S = 0x31,
  BLT_S = 0x32,
  BNE_UN_S = 0x33,
  BGE_UN_S = 0x34,
  BGT_UN_S = 0x35,
  BLE_UN_S = 0x36,
  BLT_UN_S = 0x37,
  BR = 0x38,
  BRFALSE = 0x39,
  BRTRUE = 0x3A,
  BEQ = 0x3B,
  BGE = 0x3C,
  BGT = 0x3D,
  BLE = 0x3E,
  BLT = 0x3F,
  BNE_UN = 0x40,
  BGE_UN = 0x41,
  BGT_UN = 0x42,
  BLE_UN = 0x43,
  BLT_UN = 0x44,
  SWITCH = 0x45,
  LDIND_I1 = 0x46,
  LDIND_U1 = 0x47,
  LDIND_I2 = 0x48,
  LDIND_U2 = 0x49,
  LDIND_I4 = 0x4A,
  LDIND_U4 = 0x4B,
  LDIND_I8 = 0x4C,
  LDIND_I = 0x4D,
  LDIND_R4 = 0x4E,
  LDIND_R8 = 0x4F,
  LDIND_REF = 0x50,
  STIND_REF = 0x51,
  STIND_I1 = 0x52,
  STIND_I2 = 0x53,
  STIND_I4 = 0x54,
  STIND_I8 = 0x55,
  STIND_R4 = 0x56,
  STIND_R8 = 0x57,
  ADD = 0x58,
  SUB = 0x59,
  MUL = 0x5A,
  DIV = 0x5B,
  DIV_UN = 0x5C,
  REM = 0x5D,
  REM_UN = 0x5E,
  AND = 0x5F,
  OR = 0x60,
  XOR = 0x61,
  SHL = 0x62,
  SHR = 0x63,
  SHR_UN = 0x64,
  NEG = 0x65,
  NOT = 0x66,
  CONV_I1 = 0x67,
  CONV_I2 = 0x68,
  CONV_I4 = 0x69,
  CONV_I8 = 0x6A,
  CONV_R4 = 0x6B,
  CONV_R8 = 0x6C,
  CONV_U4 = 0x6D,
  CONV_U8 = 0x6E,
  CALLVIRT = 0x6F,
  CPOBJ = 0x70,
  LDOBJ = 0x71,
  LDSTR = 0x72,
  NEWOBJ = 0x73,
  CASTCLASS = 0x74,
  ISINST = 0x75,
  CONV_R_UN = 0x76,
  UNUSED58 = 0x77,
  UNUSED1 = 0x78,
  UNBOX = 0x79,
  THROW = 0x7A,
  LDFLD = 0x7B,
  LDFLDA = 0x7C,
  STFLD = 0x7D,
  LDSFLD = 0x7E,
  LDSFLDA = 0x7F,
  STSFLD = 0x80,
  STOBJ = 0x81,
  CONV_OVF_I1_UN = 0x82,
  CONV_OVF_I2_UN = 0x83,
  CONV_OVF_I4_UN = 0x84,
  CONV_OVF_I8_UN = 0x85,
  CONV_OVF_U1_UN = 0x86,
  CONV_OVF_U2_UN = 0x87,
  CONV_OVF_U4_UN = 0x88,
  CONV_OVF_U8_UN = 0x89,
  CONV_OVF_I_UN = 0x8A,
  CONV_OVF_U_UN = 0x8B,
  BOX = 0x8C,
  NEWARR = 0x8D,
  LDLEN = 0x8E,
  LDELEMA = 0x8F,
  LDELEM_I1 = 0x90,
  LDELEM_U1 = 0x91,
  LDELEM_I2 = 0x92,
  LDELEM_U2 = 0x93,
  LDELEM_I4 = 0x94,
  LDELEM_U4 = 0x95,
  LDELEM_I8 = 0x96,
  LDELEM_I = 0x97,
  LDELEM_R4 = 0x98,
  LDELEM_R8 = 0x99,
  LDELEM_REF = 0x9A,
  STELEM_I = 0x9B,
  STELEM_I1 = 0x9C,
  STELEM_I2 = 0x9D,
  STELEM_I4 = 0x9E,
  STELEM_I8 = 0x9F,
  STELEM_R4 = 0xA0,
  STELEM_R8 = 0xA1,
  STELEM_REF = 0xA2,
  LDELEM = 0xA3,
  STELEM = 0xA4,
  UNBOX_ANY = 0xA5,
  UNUSED5 = 0xA6,
  UNUSED6 = 0xA7,
  UNUSED7 = 0xA8,
  UNUSED8 = 0xA9,
  UNUSED9 = 0xAA,
  UNUSED10 = 0xAB,
  UNUSED11 = 0xAC,
  UNUSED12 = 0xAD,
  UNUSED13 = 0xAE,
  UNUSED14 = 0xAF,
  UNUSED15 = 0xB0,
  UNUSED16 = 0xB1,
  UNUSED17 = 0xB2,
  CONV_OVF_I1 = 0xB3,
  CONV_OVF_U1 = 0xB4,
  CONV_OVF_I2 = 0xB5,
  CONV_OVF_U2 = 0xB6,
  CONV_OVF_I4 = 0xB7,
  CONV_OVF_U4 = 0xB8,
  CONV_OVF_I8 = 0xB9,
  CONV_OVF_U8 = 0xBA,
  UNUSED50 = 0xBB,
  UNUSED18 = 0xBC,
  UNUSED19 = 0xBD,
  UNUSED20 = 0xBE,
  UNUSED21 = 0xBF,
  UNUSED22 = 0xC0,
  UNUSED23 = 0xC1,
  REFANYVAL = 0xC2,
  CKFINITE = 0xC3,
  UNUSED24 = 0xC4,
  UNUSED25 = 0xC5,
  MKREFANY = 0xC6,
  UNUSED59 = 0xC7,
  UNUSED60 = 0xC8,
  UNUSED61 = 0xC9,
  UNUSED62 = 0xCA,
  UNUSED63 = 0xCB,
  UNUSED64 = 0xCC,
  UNUSED65 = 0xCD,
  UNUSED66 = 0xCE,
  UNUSED67 = 0xCF,
  LDTOKEN = 0xD0,
  CONV_U2 = 0xD1,
  CONV_U1 = 0xD2,
  CONV_I = 0xD3,
  CONV_OVF_I = 0xD4,
  CONV_OVF_U = 0xD5,
  ADD_OVF = 0xD6,
  ADD_OVF_UN = 0xD7,
  MUL_OVF = 0xD8,
  MUL_OVF_UN = 0xD9,
  SUB_OVF = 0xDA,
  SUB_OVF_UN = 0xDB,
  ENDFINALLY = 0xDC,
  LEAVE = 0xDD,
  LEAVE_S = 0xDE,
  STIND_I = 0xDF,
  CONV_U = 0xE0
};

enum OpcodeFE {
  ARGLIST = 0x00,
  CEQ = 0x01,
  CGT = 0x02,
  CGT_UN = 0x03,
  CLT = 0x04,
  CLT_UN = 0x05,
  LDFTN = 0x06,
  LDVIRTFTN = 0x07,
  UNUSED56 = 0x08,
  LDARG = 0x09,
  LDARGA = 0x0A,
  STARG = 0x0B,
  LDLOC = 0x0C,
  LDLOCA = 0x0D,
  STLOC = 0x0E,
  LOCALLOC = 0x0F,
  UNUSED57 = 0x10,
  ENDFILTER = 0x11,
  UNALIGNED_ = 0x12,
  VOLATILE_ = 0x13,
  TAIL_ = 0x14,
  INITOBJ = 0x15,
  CONSTRAINED_ = 0x16,
  CPBLK = 0x17,
  INITBLK = 0x18,
  NO_ = 0x19,
  RETHROW = 0x1A,
  UNUSED = 0x1B,
  SIZEOF = 0x1C,
  REFANYTYPE = 0x1D,
  READONLY_ = 0x1E,
  UNUSED53 = 0x1F,
  UNUSED54 = 0x20,
  UNUSED55 = 0x21,
  UNUSED70 = 0x22
};


} // end namespace n3

#endif
