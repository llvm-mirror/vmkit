//===----------- Opcodes.cpp - Reads and compiles opcodes -----------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG 0
#define N3_COMPILE 0
#define N3_EXECUTE 0

#include <cstring>

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetOptions.h>

#include "mvm/JIT.h"

#include "debug.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "CLIString.h"
#include "MSCorlib.h"
#include "N3.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

#include "OpcodeNames.def"

using namespace n3;
using namespace llvm;


static inline sint8 readS1(uint8* bytecode, uint32& i) {
  return ((sint8*)bytecode)[++i];
}

static inline uint8 readU1(uint8* bytecode, uint32& i) {
  return bytecode[++i];
}

static inline sint16 readS2(uint8* bytecode, uint32& i) {
  sint16 val = readS1(bytecode, i);
  return val | (readU1(bytecode, i) << 8);
}

static inline uint16 readU2(uint8* bytecode, uint32& i) {
  uint16 val = readU1(bytecode, i);
  return val | (readU1(bytecode, i) << 8);
}

static inline sint32 readS4(uint8* bytecode, uint32& i) {
  sint32 val = readU2(bytecode, i);
  return val | (readU2(bytecode, i) << 16);
}


static inline uint32 readU4(uint8* bytecode, uint32& i) {
  return readS4(bytecode, i);
}

static inline sint64 readS8(uint8* bytecode, uint32& i) {
  uint64 val = readU4(bytecode, i);
  uint64 _val2 = readU4(bytecode, i);
  uint64 val2 = _val2 << 32;
  return val | val2;
}

typedef union ufloat_t {
  uint32 i;
  float f;
}ufloat_t;

typedef union udouble_t {
  sint64 l;
  double d;
}udouble_t;


static inline float readFloat(uint8* bytecode, uint32& i) {
  ufloat_t tmp;
  tmp.i = readU4(bytecode, i);
  return tmp.f;
}

static inline double readDouble(uint8* bytecode, uint32& i) {
  udouble_t tmp;
  tmp.l = readS8(bytecode, i);
  return tmp.d;
}


extern "C" void n3PrintExecution(char* opcode, VMMethod* meth) {
  fprintf(stderr, "executing %s %s\n", meth->printString(), opcode);
}


static void verifyType(Value*& val1, Value*& val2, BasicBlock* currentBlock) {
  const Type* t1 = val1->getType();
  const Type* t2 = val2->getType();
  if (t1 != t2) {
    if (t1->isInteger() && t2->isInteger()) {
      if (t1->getPrimitiveSizeInBits() < t2->getPrimitiveSizeInBits()) {
        val1 = new SExtInst(val1, t2, "", currentBlock);
      } else {
        val2 = new SExtInst(val2, t1, "", currentBlock);
      }
    } else if (t1->isFloatingPoint()) {
      if (t1->getPrimitiveSizeInBits() < t2->getPrimitiveSizeInBits()) {
        val1 = new FPExtInst(val1, t2, "", currentBlock);
      } else {
        val2 = new FPExtInst(val2, t1, "", currentBlock);
      }
    } else if (isa<PointerType>(t1) && isa<PointerType>(t2)) {
      val1 = new BitCastInst(val1, VMObject::llvmType, "", currentBlock);
      val2 = new BitCastInst(val2, VMObject::llvmType, "", currentBlock);
    } else if (t1->isInteger() && t2 == PointerType::getUnqual(Type::Int8Ty)) {
      // CLI says that this is fine for some operation
      val2 = new PtrToIntInst(val2, t1, "", currentBlock);
    } else if (t2->isInteger() && t1 == PointerType::getUnqual(Type::Int8Ty)) {
      // CLI says that this is fine for some operation
      val1 = new PtrToIntInst(val1, t2, "", currentBlock);
    }
  }
}

void convertValue(Value*& val, const Type* t1, BasicBlock* currentBlock) {
  const Type* t2 = val->getType();
  if (t1 != t2) {
    if (t1->isInteger() && t2->isInteger()) {
      if (t2->getPrimitiveSizeInBits() < t1->getPrimitiveSizeInBits()) {
        val = new SExtInst(val, t1, "", currentBlock);
      } else {
        val = new TruncInst(val, t1, "", currentBlock);
      }
    } else if (t1->isFloatingPoint() && t2->isFloatingPoint()) {
      if (t2->getPrimitiveSizeInBits() < t1->getPrimitiveSizeInBits()) {
        val = new FPExtInst(val, t1, "", currentBlock);
      } else {
        val = new FPTruncInst(val, t1, "", currentBlock);
      }
    } else if (isa<PointerType>(t1) && isa<PointerType>(t2)) {
      val = new BitCastInst(val, t1, "", currentBlock);
    }
  }
}

static void store(Value* val, Value* local, bool vol, 
                  BasicBlock* currentBlock, mvm::MvmModule* module) {
  const Type* contained = local->getType()->getContainedType(0);
  if (contained->isSingleValueType()) {
    if (val->getType() != contained) {
      convertValue(val, contained, currentBlock);
    }
    new StoreInst(val, local, vol, currentBlock);
  } else if (isa<PointerType>(val->getType())) {
    uint64 size = module->getTypeSize(contained);
        
    std::vector<Value*> params;
    params.push_back(new BitCastInst(local, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
    params.push_back(new BitCastInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
    params.push_back(ConstantInt::get(Type::Int32Ty, size));
    params.push_back(module->constantZero);
    CallInst::Create(module->llvm_memcpy_i32, params.begin(), params.end(), "", currentBlock);
  } else {
    new StoreInst(val, local, vol, currentBlock);
  }
}

static Value* load(Value* val, const char* name, BasicBlock* currentBlock, mvm::MvmModule* module) {
  const Type* contained = val->getType()->getContainedType(0);
  if (contained->isSingleValueType()) {
    return new LoadInst(val, name, currentBlock);
  } else {
    uint64 size = module->getTypeSize(contained);
    Value* ret = new AllocaInst(contained, "", currentBlock); 
    std::vector<Value*> params;
    params.push_back(new BitCastInst(ret, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
    params.push_back(new BitCastInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
    params.push_back(ConstantInt::get(Type::Int32Ty, size));
    params.push_back(module->constantZero);
    CallInst::Create(module->llvm_memcpy_i32, params.begin(), params.end(), "", currentBlock);
    return ret;
  }
}

void CLIJit::compileOpcodes(uint8* bytecodes, uint32 codeLength, VMGenericClass* genClass, VMGenericMethod* genMethod) {
  uint32 leaveIndex = 0;
  bool isVolatile = false;
  for(uint32 i = 0; i < codeLength; ++i) {
    
    if (bytecodes[i] != 0xFE) {
      PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "\t[at %5x] %-5d ", i,
                  bytecodes[i]);
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "compiling %s::", compilingMethod->printString());
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]]);
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "\n");
    }
    
    Opinfo* opinfo = &(opcodeInfos[i]);
    if (opinfo->newBlock) {
      if (currentBlock->getTerminator() == 0) {
        branch(opinfo->newBlock, currentBlock);
      }
      setCurrentBlock(opinfo->newBlock);
    }
    currentExceptionBlock = opinfo->exceptionBlock;
    if (currentBlock->getTerminator() != 0) { // To prevent a gcj bug with useless goto
      currentBlock = createBasicBlock("gcj bug");
    }

#if N3_EXECUTE > 1
    if (bytecodes[i] == 0xFE) {
      std::vector<llvm::Value*> args;
      args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)OpcodeNamesFE[bytecodes[i + 1]]));
      args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
      CallInst::Create(printExecutionLLVM, args.begin(), args.end(), "", currentBlock);
    } else {
      std::vector<llvm::Value*> args;
      args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)OpcodeNames[bytecodes[i]]));
      args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
      CallInst::Create(printExecutionLLVM, args.begin(), args.end(), "", currentBlock);
    }
#endif
  
    if (opinfo->reqSuppl) {
      push(new LoadInst(supplLocal, "", currentBlock));
    }

    switch (bytecodes[i]) {
      
      case ADD: {
        Value* val2 = pop();
        bool isPointer = (val2->getType() == module->ptrType);
        Value* val1 = pop();
        isPointer |= (val1->getType() == module->ptrType);
        verifyType(val1, val2, currentBlock);
        Value* res = BinaryOperator::CreateAdd(val1, val2, "", currentBlock);
        if (isPointer) {
          res = new IntToPtrInst(res, module->ptrType, "", currentBlock);
        }
        push(res);
        break;
      }
      
      case ADD_OVF: {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case ADD_OVF_UN: {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case AND: {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateAnd(val1, val2, "", currentBlock));
        break;
      }
      
#define TEST(name, read, cmpf, cmpi, offset) case name : { \
        uint32 tmp = i;       \
        Value* val2 = pop();  \
        Value* val1 = pop();  \
        BasicBlock* ifTrue = opcodeInfos[tmp + offset + read(bytecodes, i)].newBlock; \
        Value* test = 0; \
        verifyType(val1, val2, currentBlock); \
        if (val1->getType()->isFloatingPoint()) { \
          test = new FCmpInst(FCmpInst::cmpf, val1, val2, "", currentBlock); \
        } else {  \
          test = new ICmpInst(ICmpInst::cmpi, val1, val2, "", currentBlock); \
        } \
        BasicBlock* ifFalse = createBasicBlock("false BEQ"); \
        branch(test, ifTrue, ifFalse, currentBlock); \
        currentBlock = ifFalse; \
        break; \
      }
      
      TEST(BEQ, readS4, FCMP_OEQ, ICMP_EQ, 5);
      TEST(BEQ_S, readS1, FCMP_OEQ, ICMP_EQ, 2);
      
      TEST(BGE, readS4, FCMP_OGE, ICMP_SGE, 5);
      TEST(BGE_S, readS1, FCMP_OGE, ICMP_SGE, 2);
      TEST(BGE_UN, readS4, FCMP_UGE, ICMP_UGE, 5);
      TEST(BGE_UN_S, readS1, FCMP_UGE, ICMP_UGE, 2);
      
      TEST(BGT, readS4, FCMP_OGT, ICMP_SGT, 5);
      TEST(BGT_S, readS1, FCMP_OGT, ICMP_SGT, 2);
      TEST(BGT_UN, readS4, FCMP_UGT, ICMP_UGT, 5);
      TEST(BGT_UN_S, readS1, FCMP_UGT, ICMP_UGT, 2);

      TEST(BLE, readS4, FCMP_OLE, ICMP_SLE, 5);
      TEST(BLE_S, readS1, FCMP_OLE, ICMP_SLE, 2);
      TEST(BLE_UN, readS4, FCMP_ULE, ICMP_ULE, 5);
      TEST(BLE_UN_S, readS1, FCMP_ULE, ICMP_ULE, 2);
      
      TEST(BLT, readS4, FCMP_OLT, ICMP_SLT, 5);
      TEST(BLT_S, readS1, FCMP_OLT, ICMP_SLT, 2);
      TEST(BLT_UN, readS4, FCMP_ULT, ICMP_ULT, 5);
      TEST(BLT_UN_S, readS1, FCMP_ULT, ICMP_ULT, 2);
      
      TEST(BNE_UN, readS4, FCMP_UNE, ICMP_NE, 5);
      TEST(BNE_UN_S, readS1, FCMP_UNE, ICMP_NE, 2);

#undef TEST 
      
      case BR : {
        uint32 tmp = i;
        BasicBlock* br = opcodeInfos[tmp + 5 + readS4(bytecodes, i)].newBlock;
        branch(br, currentBlock); 
        break;
      }
      
      case BR_S : {
        uint32 tmp = i;
        BasicBlock* br = opcodeInfos[tmp + 2 + readS1(bytecodes, i)].newBlock;
        branch(br, currentBlock); 
        break;
      }

      case BREAK: break;

#define TEST(name, read, cmpf, cmpi, offset) case name : { \
        uint32 tmp = i;       \
        Value* val2 = pop();  \
        Value* val1 = Constant::getNullValue(val2->getType());  \
        BasicBlock* ifTrue = opcodeInfos[tmp + offset + read(bytecodes, i)].newBlock; \
        Value* test = 0; \
        if (val1->getType()->isFloatingPoint()) { \
          test = new FCmpInst(FCmpInst::cmpf, val1, val2, "", currentBlock); \
        } else {  \
          test = new ICmpInst(ICmpInst::cmpi, val1, val2, "", currentBlock); \
        } \
        BasicBlock* ifFalse = createBasicBlock("false BR"); \
        branch(test, ifTrue, ifFalse, currentBlock); \
        currentBlock = ifFalse; \
        break; \
      }

      TEST(BRFALSE, readS4, FCMP_OEQ, ICMP_EQ, 5);
      TEST(BRFALSE_S, readS1, FCMP_OEQ, ICMP_EQ, 2);
      TEST(BRTRUE, readS4, FCMP_ONE, ICMP_NE, 5);
      TEST(BRTRUE_S, readS1, FCMP_ONE, ICMP_NE, 2);

#undef TEST
      
      case CALL: {
        uint32 value = readU4(bytecodes, i);
        invoke(value, genClass, genMethod);
        break;
      }

      case CALLI: {
        VMThread::get()->vm->unknownError("implement me");
        break;
      }

      case CKFINITE : {
        VMThread::get()->vm->unknownError("implement me");
        break;
      }

      case CONV_I1 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToSIInst(val, Type::Int8Ty, "", currentBlock));
        } else if (type == Type::Int16Ty || type == Type::Int32Ty || 
                   type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int8Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_I2 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToSIInst(val, Type::Int16Ty, "", currentBlock));
        } else if (type == Type::Int32Ty || type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int16Ty, "", currentBlock));
        } else if (type == Type::Int8Ty) {
          push(new SExtInst(val, Type::Int16Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_I4 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToSIInst(val, Type::Int32Ty, "", currentBlock));
        } else if (type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int32Ty, "", currentBlock));
        } else if (type == Type::Int8Ty || type == Type::Int16Ty) {
          push(new SExtInst(val, Type::Int32Ty, "", currentBlock));
        } else if (type == Type::Int32Ty) {
          push(val);
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_I8 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToSIInst(val, Type::Int64Ty, "", currentBlock));
        } else if (type == Type::Int8Ty || type == Type::Int16Ty || 
                   type == Type::Int32Ty) {
          push(new SExtInst(val, Type::Int64Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_R4 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::DoubleTy) {
          push(new FPTruncInst(val, Type::FloatTy, "", currentBlock));
        } else if (type->isInteger()) {
          push(new SIToFPInst(val, Type::FloatTy, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_R8 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::FloatTy) {
          push(new FPExtInst(val, Type::DoubleTy, "", currentBlock));
        } else if (type->isInteger()) {
          push(new SIToFPInst(val, Type::DoubleTy, "", currentBlock));
        } else if (type == Type::DoubleTy) {
          push(val);
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_U1 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToUIInst(val, Type::Int8Ty, "", currentBlock));
        } else if (type == Type::Int16Ty || type == Type::Int32Ty || 
                   type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int8Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_U2 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToUIInst(val, Type::Int16Ty, "", currentBlock));
        } else if (type == Type::Int32Ty || type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int8Ty, "", currentBlock));
        } else if (type == Type::Int8Ty) {
          push(new ZExtInst(val, Type::Int16Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_U4 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToUIInst(val, Type::Int32Ty, "", currentBlock));
        } else if (type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int8Ty, "", currentBlock));
        } else if (type == Type::Int8Ty || type == Type::Int16Ty) {
          push(new ZExtInst(val, Type::Int16Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_U8 : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToUIInst(val, Type::Int64Ty, "", currentBlock));
        } else if (type == Type::Int8Ty || type == Type::Int16Ty || 
                   type == Type::Int32Ty) {
          push(new ZExtInst(val, Type::Int64Ty, "", currentBlock));
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }

      case CONV_I : {
        Value* val = pop();
        Value* res = 0;
        
        if (val->getType()->isInteger()) {
          if (val->getType() != Type::Int64Ty) {
            val = new ZExtInst(val, Type::Int64Ty, "", currentBlock);
          }
          res = new IntToPtrInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock);
        } else if (!val->getType()->isFloatingPoint()) {
          res = new BitCastInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock);
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        
        push(res);
        break;
      }

      case CONV_U : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_R_UN : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::FloatTy) {
          push(new FPExtInst(val, Type::DoubleTy, "", currentBlock));
        } else if (type->isInteger()) {
          push(new UIToFPInst(val, Type::DoubleTy, "", currentBlock));
        } else if (type == Type::DoubleTy) {
          push(val);
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_OVF_I1 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I2 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I4 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I8 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U1 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U2 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U4 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U8 : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I1_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I2_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_I4_UN : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type->isFloatingPoint()) {
          push(new FPToUIInst(val, Type::Int32Ty, "", currentBlock));
        } else if (type == Type::Int64Ty) {
          push(new TruncInst(val, Type::Int8Ty, "", currentBlock));
        } else if (type == Type::Int8Ty || type == Type::Int16Ty) {
          push(new ZExtInst(val, Type::Int16Ty, "", currentBlock));
        } else if (type == Type::Int32Ty) {
          push(val);
        } else {
          VMThread::get()->vm->unknownError("implement me");
        }
        break;
      }
      
      case CONV_OVF_I8_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U1_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U2_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U4_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case CONV_OVF_U8_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      

      case DIV: {
        Value* two = pop();
        Value* one = pop();
        if (one->getType()->isFloatingPoint()) {
          convertValue(one, two->getType(), currentBlock); 
          push(BinaryOperator::CreateFDiv(one, two, "", currentBlock));
        } else {
          push(BinaryOperator::CreateSDiv(one, two, "", currentBlock));
        }
        break;
      }
      
      case DIV_UN: {
        Value* two = pop();
        Value* one = pop();
        if (one->getType()->isFloatingPoint()) {
          push(BinaryOperator::CreateFDiv(one, two, "", currentBlock));
        } else {
          push(BinaryOperator::CreateUDiv(one, two, "", currentBlock));
        }
        break;
      }

      case DUP: {
        push(top());
        break;
      }

      case ENDFINALLY : {
        Value* val = new LoadInst(supplLocal, "", currentBlock);
        val = new PtrToIntInst(val, Type::Int32Ty, "", currentBlock);
        SwitchInst* inst = SwitchInst::Create(val, leaves[0], 
                                          leaves.size(), currentBlock);
     
        uint32 index = 0; 
        for (std::vector<BasicBlock*>::iterator i = leaves.begin(), 
             e = leaves.end(); i!= e; ++i, ++index) {
          inst->addCase(ConstantInt::get(Type::Int32Ty, index), *i); 
        }

        //currentBlock = bb2;

        break;
      }
      
      case JMP : {
        VMThread::get()->vm->error("implement me"); 
        break;
      }
 
      case LDARG_S : {
        push(new LoadInst(arguments[readU1(bytecodes, i)], "", currentBlock));
        break;
      }

      case LDARG_0 : {
        push(new LoadInst(arguments[0], "", currentBlock));
        break;
      }
      
      case LDARG_1 : {
        push(new LoadInst(arguments[1], "", currentBlock));
        break;
      }
      
      case LDARG_2 : {
        push(new LoadInst(arguments[2], "", currentBlock));
        break;
      }
      
      case LDARG_3 : {
        push(new LoadInst(arguments[3], "", currentBlock));
        break;
      }

      case LDARGA_S : {
        push(arguments[readU1(bytecodes, i)]);
        break;
      }

      case LDC_I4 : {
        push(ConstantInt::get(Type::Int32Ty, readS4(bytecodes, i)));
        break;
      }
      
      case LDC_I8 : {
        push(ConstantInt::get(Type::Int64Ty, readS8(bytecodes, i)));
        break;
      }
      
      case LDC_R4 : {
        push(ConstantFP::get(Type::FloatTy, readFloat(bytecodes, i)));
        break;
      }
      
      case LDC_R8 : {
        push(ConstantFP::get(Type::DoubleTy, readDouble(bytecodes, i)));
        break;
      }
      
      case LDC_I4_0 : {
        push(module->constantZero);
        break;
      }
      
      case LDC_I4_1 : {
        push(module->constantOne);
        break;
      }
      
      case LDC_I4_2 : {
        push(module->constantTwo);
        break;
      }
      
      case LDC_I4_3 : {
        push(module->constantThree);
        break;
      }
      
      case LDC_I4_4 : {
        push(module->constantFour);
        break;
      }
      
      case LDC_I4_5 : {
        push(module->constantFive);
        break;
      }
      
      case LDC_I4_6 : {
        push(module->constantSix);
        break;
      }
      
      case LDC_I4_7 : {
        push(module->constantSeven);
        break;
      }
      
      case LDC_I4_8 : {
        push(module->constantEight);
        break;
      }
      
      case LDC_I4_M1 : {
        push(module->constantMinusOne);
        break;
      }
      
      case LDC_I4_S : {
        push(ConstantInt::get(Type::Int32Ty, readS1(bytecodes, i)));
        break;
      }
 
      case LDIND_U1 :
      case LDIND_I1 : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_U2 :
      case LDIND_I2 : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(Type::Int16Ty), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_U4 :
      case LDIND_I4 : {
        Value* val = pop();
        if (val->getType()->isInteger()) {
          val = new IntToPtrInst(val, PointerType::getUnqual(Type::Int32Ty), "", currentBlock);
        } else {
          val = new BitCastInst(val, PointerType::getUnqual(Type::Int32Ty), "", currentBlock);
        }
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_I8 : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(Type::Int64Ty), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }

      case LDIND_R4 : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(Type::FloatTy), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_R8 : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(Type::DoubleTy), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_I : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(
                                        PointerType::getUnqual(Type::Int8Ty)), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDIND_REF : {
        Value* _val = pop();
        Value* val = new BitCastInst(_val, PointerType::getUnqual(
                                        PointerType::getUnqual(VMObject::llvmType)), "", currentBlock);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
 
      case LDLOC_S : {
        Value* val = load(locals[readU1(bytecodes, i)], "", currentBlock, module);
        push(val);
        break;
      }
      
      case LDLOC_0 : {
        Value* val = load(locals[0], "", currentBlock, module);
        push(val);
        break;
      }
      
      case LDLOC_1 : {
        Value* val = load(locals[1], "", currentBlock, module);
        push(val);
        break;
      }
      
      case LDLOC_2 : {
        Value* val = load(locals[2], "", currentBlock, module);
        push(val);
        break;
      }
      
      case LDLOC_3 : {
        Value* val = load(locals[3], "", currentBlock, module);
        push(val);
        break;
      }
      
      case LDLOCA_S : {
        push(locals[readU1(bytecodes, i)]);
        break;
      }
      
      case LDNULL : {
        push(CLIJit::constantVMObjectNull);
        break;
      }

      case LEAVE : {
        uint32 tmp = i;
        uint32 index = tmp + 5 + readS4(bytecodes, i);
        BasicBlock* bb = opcodeInfos[index].newBlock;
        assert(bb);
        stack.clear();
        if (finallyHandlers.size()) {
          Exception* res = 0;
          for (std::vector<Exception*>::iterator i = finallyHandlers.begin(), 
               e = finallyHandlers.end(); i!= e; ++i) {
            Exception* ex = (*i);
            if (tmp >= ex->tryOffset && tmp < ex->tryOffset + ex->tryLength) {
              res = ex;
              break;
            }
          }
          if (res) {
            Value* expr = ConstantExpr::getIntToPtr(
                                    ConstantInt::get(Type::Int64Ty,
                                                     uint64_t (leaveIndex++)),
                                    VMObject::llvmType);

            new StoreInst(expr, supplLocal, false, currentBlock);
            branch(res->handler, currentBlock);
          } else {
            branch(bb, currentBlock);
          }
        } else {
          branch(bb, currentBlock);
        }
        break;
      }
      
      case LEAVE_S : {
        uint32 tmp = i;
        BasicBlock* bb = opcodeInfos[tmp + 2 + readS1(bytecodes, i)].newBlock;
        assert(bb);
        stack.clear();
        if (finallyHandlers.size()) {
          Exception* res = 0;
          for (std::vector<Exception*>::iterator i = finallyHandlers.begin(), 
               e = finallyHandlers.end(); i!= e; ++i) {
            Exception* ex = (*i);
            if (tmp >= ex->tryOffset && tmp < ex->tryOffset + ex->tryLength) {
              res = ex;
              break;
            }
          }
          if (res) {
            Value* expr = ConstantExpr::getIntToPtr(
                                    ConstantInt::get(Type::Int64Ty,
                                                     uint64_t (leaveIndex++)),
                                    VMObject::llvmType);

            new StoreInst(expr, supplLocal, false, currentBlock);
            branch(res->handler, currentBlock);
          } else {
            branch(bb, currentBlock);
          }
        } else {
          branch(bb, currentBlock);
        }
        break;
      }

      case MUL : {
        Value* val2 = pop();
        Value* val1 = pop();
        convertValue(val1, val2->getType(), currentBlock); 
        push(BinaryOperator::CreateMul(val1, val2, "", currentBlock));
        break;
      }

      case MUL_OVF : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case MUL_OVF_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case NEG : {
        Value* val = pop();
        push(BinaryOperator::CreateSub(
                              Constant::getNullValue(val->getType()),
                              val, "", currentBlock));
        break;
      }
      
      case NOP : break;

      case NOT : {
        push(BinaryOperator::CreateNot(pop(), "", currentBlock));
        break;
      }

      case OR : {
        Value* two = pop();
        Value* one = pop();
        push(BinaryOperator::CreateOr(one, two, "", currentBlock));
        break;
      }

      case POP : {
        pop();
        break;
      }

      case REM : {
        Value* two = pop();
        Value* one = pop();
        if (one->getType()->isFloatingPoint()) {
          push(BinaryOperator::CreateFRem(one, two, "", currentBlock));
        } else {
          push(BinaryOperator::CreateSRem(one, two, "", currentBlock));
        }
        break;
      }
      
      case REM_UN : {
        Value* two = pop();
        Value* one = pop();
        if (one->getType()->isFloatingPoint()) {
          push(BinaryOperator::CreateFRem(one, two, "", currentBlock));
        } else {
          push(BinaryOperator::CreateURem(one, two, "", currentBlock));
        }
        break;
      }

      case RET : {
        if (compilingMethod->getSignature(genMethod)->getReturnType() != Type::VoidTy) {
          Value* val = pop();
          if (val->getType() == PointerType::getUnqual(endNode->getType())) {
            // In case it's a struct
            val = new LoadInst(val, "", currentBlock);
          } else {
            convertValue(val, endNode->getType(), currentBlock);
          }
          endNode->addIncoming(val, currentBlock);
        } else if (compilingMethod->structReturn) {
          endNode->addIncoming(pop(), currentBlock);
        }
        BranchInst::Create(endBlock, currentBlock);
        break;
      }
      
      case SHL : {
        Value* val2 = pop();
        Value* val1 = pop();
        verifyType(val1, val2, currentBlock);
        push(BinaryOperator::CreateShl(val1, val2, "", currentBlock));
        break;
      }
      
      case SHR : {
        Value* val2 = pop();
        Value* val1 = pop();
        verifyType(val1, val2, currentBlock);
        push(BinaryOperator::CreateAShr(val1, val2, "", currentBlock));
        break;
      }
      
      case SHR_UN : {
        Value* val2 = pop();
        Value* val1 = pop();
        verifyType(val1, val2, currentBlock);
        push(BinaryOperator::CreateLShr(val1, val2, "", currentBlock));
        break;
      }
 
      case STARG_S : {
        Value* val = pop();
        Value* arg = arguments[readU1(bytecodes, i)];
        convertValue(val, arg->getType()->getContainedType(0), currentBlock);
        new StoreInst(val, arg, false, currentBlock);
        break;
      }

      case STIND_I1 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::Int8Ty), "",
                                      currentBlock);
        convertValue(val, Type::Int8Ty, currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_I2 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::Int16Ty), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_I4 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::Int32Ty), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_I8 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::Int64Ty), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_R4 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::FloatTy), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_R8 : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::DoubleTy), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_I : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(Type::Int32Ty), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STIND_REF : {
        Value* val = pop();
        Value* _addr = pop();
        Value* addr = new BitCastInst(_addr, PointerType::getUnqual(val->getType()), 
                                      "", currentBlock);
        new StoreInst(val, addr, isVolatile, currentBlock);
        isVolatile = false;
        break;
      }
      
      case STLOC_S : {
        Value* val = pop();
        Value* local = locals[readU1(bytecodes, i)];
        store(val, local, false, currentBlock, module);
        break;
      }
      
      case STLOC_0 : {
        Value* val = pop();
        Value* local = locals[0];
        store(val, local, false, currentBlock, module);
        break;
      }
      
      case STLOC_1 : {
        Value* val = pop();
        Value* local = locals[1];
        store(val, local, false, currentBlock, module);
        break;
      }
      
      case STLOC_2 : {
        Value* val = pop();
        Value* local = locals[2];
        store(val, local, false, currentBlock, module);
        break;
      }
      
      case STLOC_3 : {
        Value* val = pop();
        Value* local = locals[3];
        store(val, local, false, currentBlock, module);
        break;
      }
      
      case SUB : {
        Value* val2 = pop();
        Value* val1 = pop();
        verifyType(val1, val2, currentBlock);
        push(BinaryOperator::CreateSub(val1, val2, "", currentBlock));
        break;
      }

      case SUB_OVF : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case SUB_OVF_UN : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case SWITCH : {
        uint32 value = readU4(bytecodes, i);
        Value* val = pop();
        uint32 next = i + value * sizeof(sint32) + 1;
        BasicBlock* defBB = opcodeInfos[next].newBlock;
        SwitchInst* SI = SwitchInst::Create(val, defBB, value, currentBlock);
        for (uint32 t = 0; t < value; t++) {
          sint32 offset = readS4(bytecodes, i);
          sint32 index = next + offset;
          assert(index > 0);
          BasicBlock* BB = opcodeInfos[index].newBlock;
          SI->addCase(ConstantInt::get(Type::Int32Ty, t), BB);
        }
        break;
      }

      case XOR : {
        Value* two = pop();
        Value* one = pop();
        convertValue(two, one->getType(), currentBlock);
        push(BinaryOperator::CreateXor(one, two, "", currentBlock));
        break;
      }

      case BOX : {
        uint32 token = readU4(bytecodes, i);
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        VMCommonClass* type = assembly->loadType(vm, token, true, false, false,
                                                 true, genClass, genMethod);
        assert(type);
        
        if (!type->isPrimitive) {
          // the box instruction has no effect on non-primitive types
          break;
        }

        Value* var = new LoadInst(type->llvmVar(), "", currentBlock);
        Value* obj = CallInst::Create(objConsLLVM, var, "", currentBlock);
        Value* val = pop();
        obj = new BitCastInst(obj, type->virtualType, "", currentBlock);
    

        std::vector<Value*> ptrs;
        ptrs.push_back(module->constantZero);
        ptrs.push_back(module->constantOne);
        Value* ptr = GetElementPtrInst::Create(obj, ptrs.begin(), ptrs.end(), "", 
                                           currentBlock);

        if (val->getType()->getTypeID() != Type::PointerTyID) {
          convertValue(val, type->naturalType, currentBlock); 
          Value* tmp = new AllocaInst(type->naturalType, "", currentBlock);
          new StoreInst(val, tmp, false, currentBlock);
          val = tmp;
        }
        
        
        uint64 size = module->getTypeSize(type->naturalType);
        
        std::vector<Value*> params;
        params.push_back(new BitCastInst(ptr, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
        params.push_back(new BitCastInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
        params.push_back(ConstantInt::get(Type::Int32Ty, size));
        params.push_back(module->constantZero);
        CallInst::Create(module->llvm_memcpy_i32, params.begin(), params.end(), "", currentBlock);
        

        push(obj);
        break; 
      }

      case CALLVIRT : {
        uint32 value = readU4(bytecodes, i);
        invokeInterfaceOrVirtual(value, genClass, genMethod);
        break; 
      }

      case CASTCLASS : {
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        uint32 token = readU4(bytecodes, i);
        VMCommonClass* dcl = assembly->loadType(vm, token, true, false,
                                               false, true, genClass, genMethod);
        Value* obj = new BitCastInst(pop(), VMObject::llvmType, "", currentBlock);

        Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj, 
                                  CLIJit::constantVMObjectNull,
                                  "", currentBlock);
     
        BasicBlock* ifTrue = createBasicBlock("null checkcast");
        BasicBlock* ifFalse = createBasicBlock("non null checkcast");

        branch(cmp, ifTrue, ifFalse, currentBlock);

        Value* clVar = new LoadInst(dcl->llvmVar(), "", ifFalse);
        std::vector<Value*> args;
        args.push_back(obj);
        args.push_back(clVar);
        Value* call = CallInst::Create(instanceOfLLVM, args.begin(), args.end(),
                                   "", ifFalse);
     
        cmp = new ICmpInst(ICmpInst::ICMP_EQ, call,
                           module->constantZero, "", ifFalse);

        BasicBlock* ex = createBasicBlock("false checkcast");
        branch(cmp, ex, ifTrue, ifFalse);
        
        std::vector<Value*> exArgs;
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(classCastExceptionLLVM, unifiedUnreachable, currentExceptionBlock, exArgs.begin(), exArgs.end(), "", ex);
        } else {
          CallInst::Create(classCastExceptionLLVM, exArgs.begin(), exArgs.end(), "", ex);
          new UnreachableInst(ex);
        }

        currentBlock = ifTrue;
        push(new BitCastInst(obj, dcl->virtualType, "", currentBlock));
        break;
      } 
        

      case CPOBJ : {
        VMThread::get()->vm->error("implement me");
        break;  
      }
       
      case ISINST : {
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        uint32 token = readU4(bytecodes, i);
        VMCommonClass* dcl = assembly->loadType(vm, token, true, false,
                                               false, true, genClass, genMethod);
        Value* obj = pop();

        Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj, 
                                  Constant::getNullValue(obj->getType()),
                                  "", currentBlock);
        Constant* nullVirtual = Constant::getNullValue(dcl->virtualType);

     
        BasicBlock* isInstEndBlock = createBasicBlock("end isinst");
        PHINode* node = PHINode::Create(dcl->virtualType, "", isInstEndBlock);

        BasicBlock* ifFalse = createBasicBlock("non null isinst");
        BasicBlock* ifTrue = createBasicBlock("null isinst");
        
        BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
        node->addIncoming(nullVirtual, ifTrue);
        BranchInst::Create(isInstEndBlock, ifTrue);


        Value* clVar = new LoadInst(dcl->llvmVar(), "", ifFalse);
        std::vector<Value*> args;
        args.push_back(new BitCastInst(obj, VMObject::llvmType, "", ifFalse));
        args.push_back(clVar);
        Value* call = CallInst::Create(instanceOfLLVM, args.begin(), args.end(),
                                   "", ifFalse);
     
        cmp = new ICmpInst(ICmpInst::ICMP_EQ, call,
                           module->constantZero, "", ifFalse);

        BasicBlock* falseInst = createBasicBlock("false isinst");
        BasicBlock* trueInst = createBasicBlock("true isinst");
        BranchInst::Create(falseInst, trueInst, cmp, ifFalse);

        node->addIncoming(new BitCastInst(obj, dcl->virtualType, "", trueInst), trueInst);
        BranchInst::Create(isInstEndBlock, trueInst);
        
        node->addIncoming(nullVirtual, falseInst);
        BranchInst::Create(isInstEndBlock, falseInst);
       
        currentBlock = isInstEndBlock;
        push(node);
        break;
      }
      
      case LDELEM : {
        VMThread::get()->vm->error("implement me");
        break;  
      }
      
      case LDELEM_I1 : {
        VMThread::get()->vm->error("implement me");
        break;  
      }

      case LDELEM_I2 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt16::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_I4 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt32::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_I8 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayLong::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_U1 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayUInt8::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_U2 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayUInt16::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_U4 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayUInt32::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_R4 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayFloat::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_R8 : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayDouble::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_I : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt32::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }
      
      case LDELEM_REF : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayObject::llvmType);
        push(new LoadInst(ptr, "", currentBlock));
        break;
      }

      case LDELEMA : {
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        uint32 token = readU4(bytecodes, i);
        VMCommonClass* cl = assembly->loadType(vm, token, true, false,
                                               false, true, genClass, genMethod);
        VMClassArray* array = assembly->constructArray(cl, 1);
        array->resolveType(false, false, genMethod);
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, array->naturalType);
        push(ptr);
        break;
      }

      case LDFLD : {
        uint32 value = readU4(bytecodes, i);
        Value* val = getVirtualField(value, genClass, genMethod);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDFLDA : {
        uint32 value = readU4(bytecodes, i);
        push(getVirtualField(value, genClass, genMethod));
        break;
      }

      case LDLEN : {
        push(arraySize(pop()));
        break;
      }

      case LDOBJ : {
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        uint32 token = readU4(bytecodes, i);
        VMCommonClass* cl = assembly->loadType(vm, token, true, false,
                                               false, true, genClass, genMethod);
        if (!(cl->super == MSCorlib::pValue || cl->super == MSCorlib::pEnum)) {
          push(new LoadInst(pop(), "", isVolatile, currentBlock));
          isVolatile = false;
        } 
        break;
      }

      case LDSFLD : {
        uint32 value = readU4(bytecodes, i);
        Value* val = getStaticField(value, genClass, genMethod);
        push(new LoadInst(val, "", isVolatile, currentBlock));
        isVolatile = false;
        break;
      }
      
      case LDSFLDA : {
        uint32 value = readU4(bytecodes, i);
        push(getStaticField(value, genClass, genMethod));
        break;
      }

      case LDSTR : {
        uint32 value = readU4(bytecodes, i);
        uint32 index = value & 0xfffffff;
        const UTF8* utf8 = compilingClass->assembly->readUserString(index);
        Value* val = ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, (int64_t)utf8),
                                               module->ptrType);
        Value* res = CallInst::Create(newStringLLVM, val, "", currentBlock);
        /*CLIString * str = 
          (CLIString*)(((N3*)VMThread::get()->vm)->UTF8ToStr(utf8));
        GlobalVariable* gv = str->llvmVar();
        push(new BitCastInst(new LoadInst(gv, "", currentBlock), 
                             MSCorlib::pString->naturalType, "", currentBlock));*/
        push(new BitCastInst(res, MSCorlib::pString->naturalType, "", currentBlock));
        break;
      }

      case LDTOKEN : {
        uint32 token = readU4(bytecodes, i);
        uint32 table = token >> 24;
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        switch (table) {
          case CONSTANT_Field : {
            uint32 typeToken = assembly->getTypedefTokenFromField(token);
            assembly->loadType(vm, typeToken, true, true, false, true, genClass, genMethod);
            VMField* field = assembly->lookupFieldFromToken(token);
            if (!field) {
              VMThread::get()->vm->error("implement me");
            }
            Value* arg = new LoadInst(field->llvmVar(), "", currentBlock);
            push(arg);
            break;
          }
          
          case CONSTANT_MethodDef : {
            uint32 typeToken = assembly->getTypedefTokenFromMethod(token);
            assembly->loadType(vm, typeToken, true, true, false, true, genClass, genMethod);
            VMMethod* meth = assembly->lookupMethodFromToken(token);
            if (!meth) {
              VMThread::get()->vm->error("implement me");
            }
            Value* arg = new LoadInst(meth->llvmVar(), "", currentBlock);
            push(arg);
            break;
          }

          case CONSTANT_TypeDef :
          case CONSTANT_TypeRef : {
            VMCommonClass* cl = assembly->loadType(vm, token, true, false,
                                                   false, true, genClass, genMethod);
            Value* arg = new LoadInst(cl->llvmVar(), "", currentBlock);
            push(arg);
            break;
          }

          default :
            VMThread::get()->vm->error("implement me");
        }
        break;
      }
      
      case MKREFANY : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case NEWARR : {
        uint32 value = readU4(bytecodes, i);
        Assembly* ass = compilingClass->assembly;
        VMCommonClass* baseType = ass->loadType((N3*)(VMThread::get()->vm),
                                                value, true, false, false, 
                                                true, genClass, genMethod);

        VMClassArray* type = ass->constructArray(baseType, 1);
        type->resolveType(false, false, genMethod);
        Value* var = new LoadInst(type->llvmVar(), "", currentBlock);
        std::vector<Value*> args;
        args.push_back(var);
        args.push_back(pop());
        Value* val = CallInst::Create(arrayConsLLVM, args.begin(), args.end(), "",
                                  currentBlock);
        push(new BitCastInst(val, type->naturalType, "", currentBlock));
        break;
      }

      case NEWOBJ : {
        uint32 value = readU4(bytecodes, i);
        invokeNew(value, genClass, genMethod);
        break;
      }
      
      case REFANYVAL : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case STELEM : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case STELEM_I1 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt8::llvmType);
        convertValue(val, Type::Int8Ty, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STELEM_I2 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt16::llvmType);
        convertValue(val, Type::Int16Ty, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STELEM_I4 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArraySInt32::llvmType);
        convertValue(val, Type::Int32Ty, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STELEM_I8 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayLong::llvmType);
        convertValue(val, Type::Int64Ty, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STELEM_R4 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayFloat::llvmType);
        convertValue(val, Type::FloatTy, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STELEM_R8 : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayDouble::llvmType);
        convertValue(val, Type::DoubleTy, currentBlock);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case STELEM_I : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case STELEM_REF : {
        Value* val = new BitCastInst(pop(), VMObject::llvmType, "", 
                                     currentBlock);
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, ArrayObject::llvmType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case STFLD : {
        uint32 index = readU4(bytecodes, i);
        setVirtualField(index, isVolatile, genClass, genMethod);
        isVolatile = false;
        break;
      }

      case STOBJ : {
        VMThread::get()->vm->error("implement me");
        isVolatile = false;
        break;
      }

      case STSFLD : {
        uint32 index = readU4(bytecodes, i);
        setStaticField(index, isVolatile, genClass, genMethod);
        isVolatile = false;
        break;
      }

      case THROW : {
        llvm::Value* arg = pop();
        arg = new BitCastInst(arg, VMObject::llvmType, "", currentBlock);
        std::vector<Value*> args;
        args.push_back(arg);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(throwExceptionLLVM, unifiedUnreachable, currentExceptionBlock, args.begin(), args.end(), "", currentBlock);
        } else {
          CallInst::Create(throwExceptionLLVM, args.begin(), args.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        break;
      }

      case UNBOX : {
        uint32 token = readU4(bytecodes, i);
        Assembly* assembly = compilingClass->assembly;
        N3* vm = (N3*)(VMThread::get()->vm);
        VMCommonClass* type = assembly->loadType(vm, token, true, false, false,
                                                 true, genClass, genMethod);
        assert(type);

        Value* val = new AllocaInst(type->naturalType, "", currentBlock);
        Value* obj = pop();

        if (obj->getType() != type->virtualType) {
          obj = new BitCastInst(obj, type->virtualType, "", currentBlock);
        }
        
        std::vector<Value*> ptrs;
        ptrs.push_back(module->constantZero);
        ptrs.push_back(module->constantOne);
        Value* ptr = GetElementPtrInst::Create(obj, ptrs.begin(), ptrs.end(), "", 
                                           currentBlock);

        uint64 size = module->getTypeSize(type->naturalType);
        
        std::vector<Value*> params;
        params.push_back(new BitCastInst(val, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
        params.push_back(new BitCastInst(ptr, PointerType::getUnqual(Type::Int8Ty), "", currentBlock));
        params.push_back(ConstantInt::get(Type::Int32Ty, size));
        params.push_back(module->constantZero);
        CallInst::Create(module->llvm_memcpy_i32, params.begin(), params.end(), "", currentBlock);
        

        push(val);
        break;
      }
      
      case UNBOX_ANY : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case 0xFE : {
        PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "\t[at %5x] %-5d ", i,
                    bytecodes[i + 1]);
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "compiling %s::", compilingMethod->printString());
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_CYAN, OpcodeNamesFE[bytecodes[i + 1]]);
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "\n");

        switch (bytecodes[++i]) {

#define TEST(name, cmpf, cmpi) case name : { \
          Value* val2 = pop(); \
          Value* val1 = pop(); \
          Value* test = 0; \
          if (val1->getType()->isFloatingPoint()) { \
            test = new FCmpInst(FCmpInst::cmpf, val1, val2, "", currentBlock); \
          } else { \
            convertValue(val2, val1->getType(), currentBlock); \
            test = new ICmpInst(ICmpInst::cmpi, val1, val2, "", currentBlock); \
          } \
          push(test); \
          break; \
        }
      
        TEST(CEQ, FCMP_OEQ, ICMP_EQ);
        TEST(CGT, FCMP_OGT, ICMP_SGT);
        TEST(CGT_UN, FCMP_UGT, ICMP_UGT);
        TEST(CLT, FCMP_OLT, ICMP_SLT);
        TEST(CLT_UN, FCMP_ULT, ICMP_ULT);

#undef TEST
          
          case CPBLK : {
            Value* three = pop();
            Value* two = pop();
            Value* one = pop();
            std::vector<Value*> args;
            args.push_back(one);
            args.push_back(two);
            args.push_back(three);
            CallInst::Create(module->llvm_memcpy_i32,
                         args.begin(), args.end(), "", currentBlock);
            isVolatile = false;
            break;
          }
      
          case ENDFILTER: {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case LDARG : {
            push(new LoadInst(arguments[readU2(bytecodes, i)], "", currentBlock));
            break;
          }
          
          case LDARGA : {
            push(arguments[readU2(bytecodes, i)]);
            break;
          }
          
          case LDFTN : {
            Assembly* assembly = compilingClass->assembly;
            N3* vm = (N3*)(VMThread::get()->vm);
            uint32 token = readU4(bytecodes, i);
            uint32 table = token >> 24;
            if (table == CONSTANT_MethodDef) {
              uint32 typeToken = assembly->getTypedefTokenFromMethod(token);
              assembly->loadType(vm, typeToken, true, false,  false, true, genClass, genMethod);
              VMMethod* meth = assembly->lookupMethodFromToken(token);
              if (!meth) VMThread::get()->vm->error("implement me");
              Value* arg = new LoadInst(meth->llvmVar(), "", currentBlock);
              push(arg);
            } else {
              VMThread::get()->vm->error("implement me");
            }
            break;
          }

          case INITBLK : {
            Value* three = pop();
            Value* two = pop();
            Value* one = pop();
            std::vector<Value*> args;
            args.push_back(one);
            args.push_back(two);
            args.push_back(three);
            CallInst::Create(module->llvm_memset_i32,
                         args.begin(), args.end(), "", currentBlock);
            isVolatile = false;
            break;
          }
      
          case LDLOC : {
            push(new LoadInst(locals[readU2(bytecodes, i)], "", currentBlock));
            break;
          }
          
          case LOCALLOC : {
            push(new AllocaInst(Type::Int8Ty, pop(), "", currentBlock));
            break;
          }
          
          case STARG : {
            new StoreInst(pop(), arguments[readU2(bytecodes, i)], false, currentBlock);
            break;
          }
          
          case STLOC : {
            Value* val = pop();
            Value* local = locals[readU2(bytecodes, i)];
            store(val, local, false, currentBlock, module);
            break;
          }
          
          case LDLOCA : {
            push(locals[readU2(bytecodes, i)]);
            break;
          }
      
          case ARGLIST : {
            VMThread::get()->vm->error("implement me");
            break;
          }
      
          case INITOBJ : {
            uint32 token = readU4(bytecodes, i);
            Assembly* assembly = compilingClass->assembly;
            N3* vm = (N3*)(VMThread::get()->vm);
            VMCommonClass* type = assembly->loadType(vm, token, true, false, false,
                                                     true, genClass, genMethod);
            if (type->super == MSCorlib::pValue) {
              uint64 size = module->getTypeSize(type->naturalType);
        
              std::vector<Value*> params;
              params.push_back(new BitCastInst(pop(), module->ptrType, "",
                                               currentBlock));
              params.push_back(module->constantInt8Zero);
              params.push_back(ConstantInt::get(Type::Int32Ty, size));
              params.push_back(module->constantZero);
              CallInst::Create(module->llvm_memset_i32, params.begin(),
                               params.end(), "", currentBlock);
            }

            break;  
          }
          
          case LDVIRTFTN : {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case REFANYTYPE : {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case RETHROW : {
            std::vector<Value*> args;
            if (opinfo->exception) {
              args.push_back(opinfo->exception);
            } else {
              args.push_back(CLIJit::constantVMObjectNull);
            }
            if (currentExceptionBlock != endExceptionBlock) {
              InvokeInst::Create(throwExceptionLLVM, unifiedUnreachable, currentExceptionBlock, args.begin(), args.end(), "", currentBlock);
            } else {
              CallInst::Create(throwExceptionLLVM, args.begin(), args.end(), "", currentBlock);
              new UnreachableInst(currentBlock);
            }
            break;
          }
        
          case SIZEOF : {
            VMThread::get()->vm->error("implement me");
            break;
          }

          case VOLATILE_ : {
            isVolatile = true;
            break;
          }
          default :
            VMThread::get()->vm->unknownError("unknown bytecode");
        } 
        break;
      }

      default :
        VMThread::get()->vm->unknownError("unknown bytecode");
    } 
  }
}

void CLIJit::exploreOpcodes(uint8* bytecodes, uint32 codeLength) {
  for(uint32 i = 0; i < codeLength; ++i) {
    
    if (bytecodes[i] != 0xFE) {
      PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "\t[at %5x] %-5d ", i,
                  bytecodes[i]);
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "exploring %s::", compilingMethod->printString());
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]]);
      PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "\n");
    }
    
    switch (bytecodes[i]) {
      
      case ADD:
      case ADD_OVF:
      case ADD_OVF_UN:
      case AND: break;
      
#define TEST(name, read, offset) case name : { \
        uint32 tmp = i; \
        uint16 index = tmp + offset + read(bytecodes, i); \
        if (!(opcodeInfos[index].newBlock)) \
          opcodeInfos[index].newBlock = createBasicBlock("Branches"); \
        break; \
      }
      
      TEST(BEQ, readS4, 5);
      TEST(BEQ_S, readS1, 2);
      
      TEST(BGE, readS4, 5);
      TEST(BGE_S, readS1, 2);
      TEST(BGE_UN, readS4, 5);
      TEST(BGE_UN_S, readS1, 2);
      
      TEST(BGT, readS4, 5);
      TEST(BGT_S, readS1, 2);
      TEST(BGT_UN, readS4, 5);
      TEST(BGT_UN_S, readS1, 2);

      TEST(BLE, readS4, 5);
      TEST(BLE_S, readS1, 2);
      TEST(BLE_UN, readS4, 5);
      TEST(BLE_UN_S, readS1, 2);
      
      TEST(BLT, readS4, 5);
      TEST(BLT_S, readS1, 2);
      TEST(BLT_UN, readS4, 5);
      TEST(BLT_UN_S, readS1, 2);
      
      TEST(BNE_UN, readS4, 5);
      TEST(BNE_UN_S, readS1, 2);

      case BR : {
        uint32 tmp = i;
        uint16 index = tmp + 5 + readS4(bytecodes, i);
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("BR");
        break;
      }
      
      case BR_S : {
        uint32 tmp = i;
        uint16 index = tmp + 2 + readS1(bytecodes, i);
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("BR");
        break;
      }

      case BREAK: break;

      TEST(BRFALSE, readS4, 5);
      TEST(BRFALSE_S, readS1, 2);
      TEST(BRTRUE, readS4, 5);
      TEST(BRTRUE_S, readS1, 2);

#undef TEST
      
      case CALL: {
        i+= 4;
        break;
      }

      case CALLI: {
        VMThread::get()->vm->unknownError("implement me");
        break;
      }

      case CKFINITE : {
        VMThread::get()->vm->unknownError("implement me");
        break;
      }

      case CONV_I1 :
      case CONV_I2 :
      case CONV_I4 :
      case CONV_I8 :
      case CONV_R4 :
      case CONV_R8 :
      case CONV_U1 :
      case CONV_U2 :
      case CONV_U4 :
      case CONV_U8 :
      case CONV_I :
      case CONV_U :
      case CONV_R_UN :
      case CONV_OVF_I1 :
      case CONV_OVF_I2 :
      case CONV_OVF_I4 :
      case CONV_OVF_I8 :
      case CONV_OVF_U1 :
      case CONV_OVF_U2 :
      case CONV_OVF_U4 :
      case CONV_OVF_U8 :
      case CONV_OVF_I :
      case CONV_OVF_U :
      case CONV_OVF_I1_UN :
      case CONV_OVF_I2_UN :
      case CONV_OVF_I4_UN :
      case CONV_OVF_I8_UN :
      case CONV_OVF_U1_UN :
      case CONV_OVF_U2_UN :
      case CONV_OVF_U4_UN :
      case CONV_OVF_U8_UN :
      case DIV:
      case DIV_UN:
      case DUP:
      case ENDFINALLY : break;
      
      case JMP : {
        VMThread::get()->vm->error("implement me"); 
        break;
      }
 
      case LDARG_S : {
        i+= 1;
        break;
      }

      case LDARG_0 :
      case LDARG_1 :
      case LDARG_2 :
      case LDARG_3 : break;

      case LDARGA_S : {
        i += 1;
        break;
      }

      case LDC_I4 : {
        i += 4;
        break;
      }
      
      case LDC_I8 : {
        i += 8;
        break;
      }
      
      case LDC_R4 : {
        i += 4;
        break;
      }
      
      case LDC_R8 : {
        i += 8;
        break;
      }
      
      case LDC_I4_0 :
      case LDC_I4_1 :
      case LDC_I4_2 :
      case LDC_I4_3 :
      case LDC_I4_4 :
      case LDC_I4_5 :
      case LDC_I4_6 :
      case LDC_I4_7 :
      case LDC_I4_8 :
      case LDC_I4_M1 : break;
      
      case LDC_I4_S : {
        i += 1;
        break;
      }
 
      case LDIND_U1 :
      case LDIND_I1 :
      case LDIND_U2 :
      case LDIND_I2 :
      case LDIND_U4 :
      case LDIND_I4 :
      case LDIND_I8 :
      case LDIND_R4 :
      case LDIND_R8 :
      case LDIND_I : break;
      
      case LDIND_REF : {
        break;
      }
 
      case LDLOC_S : {
        i += 1;
        break;
      }
      
      case LDLOC_0 :
      case LDLOC_1 :
      case LDLOC_2 :
      case LDLOC_3 : break;
      
      case LDLOCA_S : {
        i += 1;
        break;
      }
      
      case LDNULL : break;

      case LEAVE : {
        uint32 tmp = i;
        uint32 value = readS4(bytecodes, i);
        uint32 index = tmp + 5 + value;
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("LEAVE");
        leaves.push_back(opcodeInfos[index].newBlock);
        break;
      }
      
      case LEAVE_S : {
        uint32 tmp = i;
        uint8 value = readS1(bytecodes, i);
        uint32 index = tmp + 2 + value;
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("LEAVE_S");
        leaves.push_back(opcodeInfos[index].newBlock);
        break;
      }

      case MUL :
      case MUL_OVF :
      case MUL_OVF_UN :
      case NEG :
      case NOP :
      case NOT :
      case OR :
      case POP :
      case REM :
      case REM_UN :
      case RET :
      case SHL :
      case SHR :
      case SHR_UN : break;
      
      case STARG_S : {
        i += 1;
        break;
      }

      case STIND_I1 :
      case STIND_I2 :
      case STIND_I4 :
      case STIND_I8 :
      case STIND_R4 :
      case STIND_R8 :
      case STIND_I :
      case STIND_REF : break;
      
      case STLOC_S : {
        i += 1;
        break;
      }
      
      case STLOC_0 :
      case STLOC_1 :
      case STLOC_2 :
      case STLOC_3 :
      case SUB :
      case SUB_OVF :
      case SUB_OVF_UN : break;

      case SWITCH : {
        uint32 value = readU4(bytecodes, i);
        uint32 next = i + value * sizeof(sint32) + 1;
        for (uint32 t = 0; t < value; t++) {
          sint32 offset = readS4(bytecodes, i);
          sint32 index = next + offset;
          assert(index > 0);
          if (!(opcodeInfos[index].newBlock)) {
            BasicBlock* block = createBasicBlock("switch");
            opcodeInfos[index].newBlock = block;
          }
        }
        if (!(opcodeInfos[i + 1].newBlock)) {
          BasicBlock* block = createBasicBlock("switch");
          opcodeInfos[i + 1].newBlock = block;
        }
        break;
      }

      case XOR : break;

      case BOX : {
        i += 4;
        break; 
      }

      case CALLVIRT : {
        i += 4;
        break; 
      }

      case CASTCLASS : {
        i += 4;
        break;  
      }

      case CPOBJ : {
        VMThread::get()->vm->error("implement me");
        break;  
      }
       
      case ISINST : {
        i += 4;
        break;  
      }
      
      case LDELEM : {
        VMThread::get()->vm->error("implement me");
        break;  
      }
      
      case LDELEM_I1 : {
        VMThread::get()->vm->error("implement me");
        break;  
      }

      case LDELEM_I2 :
      case LDELEM_I4 :
      case LDELEM_I8 :
      case LDELEM_U1 :
      case LDELEM_U2 :
      case LDELEM_U4 :
      case LDELEM_R4 :
      case LDELEM_R8 :
      case LDELEM_I :
      case LDELEM_REF :
      case LDELEMA : break;

      case LDFLD : {
        i += 4;
        break;
      }
      
      case LDFLDA : {
        i += 4;
        break;
      }

      case LDLEN : break;

      case LDOBJ : {
        i += 4;
        break;
      }

      case LDSFLD : {
        i += 4;
        break;
      }
      
      case LDSFLDA : {
        i += 4;
        break;
      }

      case LDSTR : {
        i += 4;
        break;
      }

      case LDTOKEN : {
        i += 4;
        break;
      }
      
      case MKREFANY : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case NEWARR : {
        i += 4;
        break;
      }

      case NEWOBJ : {
        i += 4;
        break;
      }
      
      case REFANYVAL : {
        VMThread::get()->vm->error("implement me");
        break;
      }
      
      case STELEM : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case STELEM_I1 :
      case STELEM_I2 :
      case STELEM_I4 :
      case STELEM_I8 :
      case STELEM_R4 :
      case STELEM_R8 :
      case STELEM_I :
      case STELEM_REF : break;

      case STFLD : {
        i += 4;
        break;
      }

      case STOBJ : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case STSFLD : {
        i += 4;
        break;
      }

      case THROW : {
        break;
      }

      case UNBOX : {
        i += 4;
        break;
      }
      
      case UNBOX_ANY : {
        VMThread::get()->vm->error("implement me");
        break;
      }

      case 0xFE : {
      
        PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "\t[at %5x] %-5d ", i,
                    bytecodes[i + 1]);
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "exploring %s::", compilingMethod->printString());
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_CYAN, OpcodeNamesFE[bytecodes[i + 1]]);
        PRINT_DEBUG(N3_COMPILE, 1, LIGHT_BLUE, "\n");
      
        switch (bytecodes[++i]) {

          case CEQ:
          case CGT:
          case CGT_UN:
          case CLT:
          case CLT_UN:
          case CPBLK : break;
      
          case ENDFILTER: {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case LDARG : {
            i += 2;
            break;
          }
          
          case LDARGA : {
            i += 2;
            break;
          }
          
          case LDFTN : {
            i += 4;
            break;
          }

          case INITBLK : break;
      
          case LDLOC : {
            i += 2;
            break;
          }
          
          case LOCALLOC : break;
          
          case STARG : {
            i += 2;
            break;
          }
          
          case STLOC : {
            i += 2;
            break;
          }
          
          case LDLOCA : {
            i += 2;
            break;
          }
      
          case ARGLIST : {
            VMThread::get()->vm->error("implement me");
            break;
          }
      
          case INITOBJ : {
            i += 4;
            break;  
          }
          
          case LDVIRTFTN : {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case REFANYTYPE : {
            VMThread::get()->vm->error("implement me");
            break;
          }
          
          case RETHROW : {
            break;
          }
        
          case SIZEOF : {
            VMThread::get()->vm->error("implement me");
            break;
          }

          case VOLATILE_ : {
            break;
          }
          default :
            VMThread::get()->vm->unknownError("unknown bytecode");
        } 
        break;
      }
      
      default :
        VMThread::get()->vm->unknownError("unknown bytecode");
    }
  }
}
