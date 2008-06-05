//===---- JavaJITOpcodes.cpp - Reads and compiles opcodes -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG 0
#define JNJVM_COMPILE 0
#define JNJVM_EXECUTE 0

#include <string.h>

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "debug.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "OpcodeNames.def"

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;
using namespace llvm;


static inline sint8 readS1(uint8* bytecode, uint32& i) {
  return ((sint8*)bytecode)[++i];
}

static inline uint8 readU1(uint8* bytecode, uint32& i) {
  return bytecode[++i];
}

static inline sint16 readS2(uint8* bytecode, uint32& i) {
  sint16 val = readS1(bytecode, i) << 8;
  return val | readU1(bytecode, i);
}

static inline uint16 readU2(uint8* bytecode, uint32& i) {
  uint16 val = readU1(bytecode, i) << 8;
  return val | readU1(bytecode, i);
}

static inline sint32 readS4(uint8* bytecode, uint32& i) {
  sint32 val = readU2(bytecode, i) << 16;
  return val | readU2(bytecode, i);
}


static inline uint32 readU4(uint8* bytecode, uint32& i) {
  return readS4(bytecode, i);
}

uint32 JavaJIT::WREAD_U1(uint8* array, bool init, uint32 &i) {
  if (wide) {
    wide = init; 
    return readU2(array, i);
  } else {
    return readU1(array, i);
  }
}

sint32 JavaJIT::WREAD_S1(uint8* array, bool init, uint32 &i) {
  if (wide) {
    wide = init; 
    return readS2(array, i);
  } else {
    return readS1(array, i);
  }
}

uint32 JavaJIT::WCALC(uint32 n) {
  if (wide) {
    wide = false;
    return n << 1;
  } else {
    return n;
  }
}

void JavaJIT::compileOpcodes(uint8* bytecodes, uint32 codeLength) {
  wide = false;
  uint32 jsrIndex = 0;
  for(uint32 i = 0; i < codeLength; ++i) {
    
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "\t[at %5d] %-5d ", i,
                bytecodes[i]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "compiling ");
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n");
    
    Opinfo* opinfo = &(opcodeInfos[i]);
    if (opinfo->newBlock) {
      if (currentBlock->getTerminator() == 0) {
        branch(opinfo->newBlock, currentBlock);
      }
      setCurrentBlock(opinfo->newBlock);
    }
    currentExceptionBlock = opinfo->exceptionBlock;
    
    // To prevent a gcj bug with useless goto
    if (currentBlock->getTerminator() != 0) { 
      currentBlock = createBasicBlock("gcj bug");
    }
#if JNJVM_EXECUTE > 1
    {
    std::vector<llvm::Value*> args;
    args.push_back(ConstantInt::get(Type::Int32Ty, 
                                    (int64_t)OpcodeNames[bytecodes[i]]));
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)i));
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
    CallInst::Create(JnjvmModule::PrintExecutionFunction, args.begin(),
                     args.end(), "", currentBlock);
    }
#endif

    if (opinfo->reqSuppl) {
      push(new LoadInst(supplLocal, "", currentBlock), AssessorDesc::dRef);
    }

    switch (bytecodes[i]) {
      
      case ACONST_NULL : 
        push(JnjvmModule::JavaObjectNullConstant, AssessorDesc::dRef);
        break;

      case ICONST_M1 :
        push(mvm::jit::constantMinusOne, AssessorDesc::dInt);
        break;

      case ICONST_0 :
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case ICONST_1 :
        push(mvm::jit::constantOne, AssessorDesc::dInt);
        break;

      case ICONST_2 :
        push(mvm::jit::constantTwo, AssessorDesc::dInt);
        break;

      case ICONST_3 :
        push(mvm::jit::constantThree, AssessorDesc::dInt);
        break;

      case ICONST_4 :
        push(mvm::jit::constantFour, AssessorDesc::dInt);
        break;

      case ICONST_5 :
        push(mvm::jit::constantFive, AssessorDesc::dInt);
        break;

      case LCONST_0 :
        push(mvm::jit::constantLongZero, AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case LCONST_1 :
        push(mvm::jit::constantLongOne, AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case FCONST_0 :
        push(mvm::jit::constantFloatZero, AssessorDesc::dFloat);
        break;

      case FCONST_1 :
        push(mvm::jit::constantFloatOne, AssessorDesc::dFloat);
        break;
      
      case FCONST_2 :
        push(mvm::jit::constantFloatTwo, AssessorDesc::dFloat);
        break;
      
      case DCONST_0 :
        push(mvm::jit::constantDoubleZero, AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case DCONST_1 :
        push(mvm::jit::constantDoubleOne, AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case BIPUSH : 
        push(ConstantExpr::getSExt(ConstantInt::get(Type::Int8Ty,
                                                    bytecodes[++i]),
                                   Type::Int32Ty), AssessorDesc::dInt);
        break;

      case SIPUSH :
        push(ConstantExpr::getSExt(ConstantInt::get(Type::Int16Ty,
                                                    readS2(bytecodes, i)),
                                   Type::Int32Ty), AssessorDesc::dInt);
        break;

      case LDC :
        _ldc(bytecodes[++i]);
        break;

      case LDC_W :
        _ldc(readS2(bytecodes, i));
        break;

      case LDC2_W :
        _ldc(readS2(bytecodes, i));
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case ILOAD :
        push(new LoadInst(intLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), AssessorDesc::dInt);
        break;

      case LLOAD :
        push(new LoadInst(longLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case FLOAD :
        push(new LoadInst(floatLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), AssessorDesc::dFloat);
        break;

      case DLOAD :
        push(new LoadInst(doubleLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case ALOAD :
        push(new LoadInst(objectLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), AssessorDesc::dRef);
        break;
      
      case ILOAD_0 :
        push(new LoadInst(intLocals[0], "", currentBlock), AssessorDesc::dInt);
        break;
      
      case ILOAD_1 :
        push(new LoadInst(intLocals[1], "", currentBlock), AssessorDesc::dInt);
        break;

      case ILOAD_2 :
        push(new LoadInst(intLocals[2], "", currentBlock), AssessorDesc::dInt);
        break;

      case ILOAD_3 :
        push(new LoadInst(intLocals[3], "", currentBlock), AssessorDesc::dInt);
        break;
      
      case LLOAD_0 :
        push(new LoadInst(longLocals[0], "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case LLOAD_1 :
        push(new LoadInst(longLocals[1], "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case LLOAD_2 :
        push(new LoadInst(longLocals[2], "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case LLOAD_3 :
        push(new LoadInst(longLocals[3], "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case FLOAD_0 :
        push(new LoadInst(floatLocals[0], "", currentBlock),
             AssessorDesc::dFloat);
        break;
      
      case FLOAD_1 :
        push(new LoadInst(floatLocals[1], "", currentBlock),
             AssessorDesc::dFloat);
        break;

      case FLOAD_2 :
        push(new LoadInst(floatLocals[2], "", currentBlock),
             AssessorDesc::dFloat);
        break;

      case FLOAD_3 :
        push(new LoadInst(floatLocals[3], "", currentBlock),
             AssessorDesc::dFloat);
        break;
      
      case DLOAD_0 :
        push(new LoadInst(doubleLocals[0], "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case DLOAD_1 :
        push(new LoadInst(doubleLocals[1], "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case DLOAD_2 :
        push(new LoadInst(doubleLocals[2], "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case DLOAD_3 :
        push(new LoadInst(doubleLocals[3], "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case ALOAD_0 :
        push(new LoadInst(objectLocals[0], "", currentBlock),
             AssessorDesc::dRef);
        break;
      
      case ALOAD_1 :
        push(new LoadInst(objectLocals[1], "", currentBlock),
             AssessorDesc::dRef);
        break;

      case ALOAD_2 :
        push(new LoadInst(objectLocals[2], "", currentBlock),
             AssessorDesc::dRef);
        break;

      case ALOAD_3 :
        push(new LoadInst(objectLocals[3], "", currentBlock),
             AssessorDesc::dRef);
        break;
      
      case IALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, 
                                         JnjvmModule::JavaArraySInt32Type);
        push(new LoadInst(ptr, "", currentBlock), AssessorDesc::dInt);
        break;
      }

      case LALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayLongType);
        push(new LoadInst(ptr, "", currentBlock), AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayFloatType);
        push(new LoadInst(ptr, "", currentBlock), AssessorDesc::dFloat);
        break;
      }

      case DALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayDoubleType);
        push(new LoadInst(ptr, "", currentBlock), AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case AALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayObjectType);
        push(new LoadInst(ptr, "", currentBlock), AssessorDesc::dRef);
        break;
      }

      case BALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArraySInt8Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case CALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayUInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new ZExtInst(val, Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case SALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArraySInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case ISTORE : {
        Value* val = popAsInt();
        new StoreInst(val, intLocals[WREAD_U1(bytecodes, false, i)], false,
                      currentBlock);
        break;
      }
      
      case LSTORE :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[WREAD_U1(bytecodes, false, i)], false,
                      currentBlock);
        break;
      
      case FSTORE :
        new StoreInst(pop(), floatLocals[WREAD_U1(bytecodes, false, i)], false,
                      currentBlock);
        break;
      
      case DSTORE :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[WREAD_U1(bytecodes, false, i)], false,
                      currentBlock);
        break;

      case ASTORE :
        new StoreInst(pop(), objectLocals[WREAD_U1(bytecodes, false, i)], false,
                      currentBlock);
        break;
      
      case ISTORE_0 : {
        Value* val = pop();
        if (val->getType() != Type::Int32Ty) // int8 and int16
          val = new ZExtInst(val, Type::Int32Ty, "", currentBlock);
        new StoreInst(val, intLocals[0], false, currentBlock);
        break;
      }
      
      case ISTORE_1 : {
        Value* val = pop();
        if (val->getType() != Type::Int32Ty) // int8 and int16
          val = new ZExtInst(val, Type::Int32Ty, "", currentBlock);
        new StoreInst(val, intLocals[1], false, currentBlock);
        break;
      }

      case ISTORE_2 : {
        Value* val = pop();
        if (val->getType() != Type::Int32Ty) // int8 and int16
          val = new ZExtInst(val, Type::Int32Ty, "", currentBlock);
        new StoreInst(val, intLocals[2], false, currentBlock);
        break;
      }

      case ISTORE_3 : {
        Value* val = pop();
        if (val->getType() != Type::Int32Ty) // int8 and int16
          val = new ZExtInst(val, Type::Int32Ty, "", currentBlock);
        new StoreInst(val, intLocals[3], false, currentBlock);
        break;
      }

      case LSTORE_0 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[0], false, currentBlock);
        break;
      
      case LSTORE_1 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[1], false, currentBlock);
        break;
      
      case LSTORE_2 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[2], false, currentBlock);
        break;
      
      case LSTORE_3 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[3], false, currentBlock);
        break;
      
      case FSTORE_0 :
        new StoreInst(pop(), floatLocals[0], false, currentBlock);
        break;
      
      case FSTORE_1 :
        new StoreInst(pop(), floatLocals[1], false, currentBlock);
        break;
      
      case FSTORE_2 :
        new StoreInst(pop(), floatLocals[2], false, currentBlock);
        break;
      
      case FSTORE_3 :
        new StoreInst(pop(), floatLocals[3], false, currentBlock);
        break;
      
      case DSTORE_0 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[0], false, currentBlock);
        break;
      
      case DSTORE_1 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[1], false, currentBlock);
        break;
      
      case DSTORE_2 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[2], false, currentBlock);
        break;
      
      case DSTORE_3 :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[3], false, currentBlock);
        break;
      
      case ASTORE_0 :
        new StoreInst(pop(), objectLocals[0], false, currentBlock);
        break;
      
      case ASTORE_1 :
        new StoreInst(pop(), objectLocals[1], false, currentBlock);
        break;
      
      case ASTORE_2 :
        new StoreInst(pop(), objectLocals[2], false, currentBlock);
        break;
      
      case ASTORE_3 :
        new StoreInst(pop(), objectLocals[3], false, currentBlock);
        break;

      case IASTORE : {
        Value* val = popAsInt();
        Value* index = popAsInt();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArraySInt32Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case LASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayLongType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case FASTORE : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayFloatType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case DASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayDoubleType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case AASTORE : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayObjectType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case BASTORE : {
        Value* val = pop();
        if (val->getType() != Type::Int8Ty) {
          val = new TruncInst(val, Type::Int8Ty, "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArraySInt8Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case CASTORE : {
        const AssessorDesc* ass = topFunc();
        Value* val = pop();
        if (ass == AssessorDesc::dInt) {
          val = new TruncInst(val, Type::Int16Ty, "", currentBlock);
        } else if (ass == AssessorDesc::dByte || ass == AssessorDesc::dBool) {
          val = new ZExtInst(val, Type::Int16Ty, "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArrayUInt16Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case SASTORE : {
        const AssessorDesc* ass = topFunc();
        Value* val = pop();
        if (ass == AssessorDesc::dInt) {
          val = new TruncInst(val, Type::Int16Ty, "", currentBlock);
        } else if (ass == AssessorDesc::dByte || ass == AssessorDesc::dBool) {
          val = new SExtInst(val, Type::Int16Ty, "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         JnjvmModule::JavaArraySInt16Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case POP :
        pop();
        break;

      case POP2 :
        pop(); pop();
        break;

      case DUP :
        push(top(), topFunc());
        break;

      case DUP_X1 : {
        std::pair<Value*, const AssessorDesc*> one = popPair();
        std::pair<Value*, const AssessorDesc*> two = popPair();
        push(one);
        push(two);
        push(one);
        break;
      }

      case DUP_X2 : {
        std::pair<Value*, const AssessorDesc*> one = popPair();
        std::pair<Value*, const AssessorDesc*> two = popPair();
        std::pair<Value*, const AssessorDesc*> three = popPair();
        push(one);
        push(three);
        push(two);
        push(one);
        break;
      }

      case DUP2 :
        push(stack[stackSize() - 2]);
        push(stack[stackSize() - 2]);
        break;

      case DUP2_X1 : {
        std::pair<Value*, const AssessorDesc*> one = popPair();
        std::pair<Value*, const AssessorDesc*> two = popPair();
        std::pair<Value*, const AssessorDesc*> three = popPair();

        push(two);
        push(one);

        push(three);
        push(two);
        push(one);

        break;
      }

      case DUP2_X2 : {
        std::pair<Value*, const AssessorDesc*> one = popPair();
        std::pair<Value*, const AssessorDesc*> two = popPair();
        std::pair<Value*, const AssessorDesc*> three = popPair();
        std::pair<Value*, const AssessorDesc*> four = popPair();

        push(two);
        push(one);
        
        push(four);
        push(three);
        push(two);
        push(one);

        break;
      }

      case SWAP : {
        std::pair<Value*, const AssessorDesc*> one = popPair();
        std::pair<Value*, const AssessorDesc*> two = popPair();
        push(one);
        push(two);
        break;
      }

      case IADD : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createAdd(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LADD : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createAdd(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FADD : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::createAdd(val1, val2, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      }

      case DADD : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createAdd(val1, val2, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case ISUB : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createSub(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }
      case LSUB : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createSub(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FSUB : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::createSub(val1, val2, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      }

      case DSUB : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createSub(val1, val2, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IMUL : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createMul(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LMUL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createMul(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FMUL : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::createMul(val1, val2, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      }

      case DMUL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createMul(val1, val2, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IDIV : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createSDiv(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LDIV : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createSDiv(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FDIV : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::createFDiv(val1, val2, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      }

      case DDIV : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createFDiv(val1, val2, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IREM : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createSRem(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LREM : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createSRem(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FREM : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::createFRem(val1, val2, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      }

      case DREM : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::createFRem(val1, val2, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case INEG :
        push(BinaryOperator::createSub(
                              mvm::jit::constantZero,
                              popAsInt(), "", currentBlock),
             AssessorDesc::dInt);
        break;
      
      case LNEG : {
        pop();
        push(BinaryOperator::createSub(
                              mvm::jit::constantLongZero,
                              pop(), "", currentBlock), AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case FNEG :
        push(BinaryOperator::createSub(
                              mvm::jit::constantFloatMinusZero,
                              pop(), "", currentBlock), AssessorDesc::dFloat);
        break;
      
      case DNEG : {
        pop();
        push(BinaryOperator::createSub(
                              mvm::jit::constantDoubleMinusZero,
                              pop(), "", currentBlock), AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case ISHL : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createShl(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LSHL : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createShl(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case ISHR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createAShr(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LSHR : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createAShr(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IUSHR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createLShr(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LUSHR : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createLShr(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IAND : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createAnd(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LAND : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createAnd(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IOR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createOr(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LOR : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createOr(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IXOR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::createXor(val1, val2, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LXOR : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::createXor(val1, val2, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case IINC : {
        uint16 idx = WREAD_U1(bytecodes, true, i);
        sint16 val = WREAD_S1(bytecodes, false, i);
        llvm::Value* add = BinaryOperator::createAdd(
            new LoadInst(intLocals[idx], "", currentBlock), 
            ConstantInt::get(Type::Int32Ty, val), "",
            currentBlock);
        new StoreInst(add, intLocals[idx], false, currentBlock);
        break;
      }

      case I2L :
        push(new SExtInst(pop(), llvm::Type::Int64Ty, "", currentBlock),
             AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;

      case I2F :
        push(new SIToFPInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             AssessorDesc::dFloat);
        break;
        
      case I2D :
        push(new SIToFPInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case L2I :
        pop();
        push(new TruncInst(pop(), llvm::Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      
      case L2F :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             AssessorDesc::dFloat);
        break;
      
      case L2D :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case F2I : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("F2I");
        PHINode* node = PHINode::Create(llvm::Type::Int32Ty, "", res);
        node->addIncoming(mvm::jit::constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val, 
                            mvm::jit::constantMaxIntFloat,
                            "", currentBlock);

        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMaxInt,
                          currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val,
                            mvm::jit::constantMinIntFloat,
                            "", currentBlock);
        
        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int32Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;

        push(node, AssessorDesc::dInt);
        break;
      }

      case F2L : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("F2L");
        PHINode* node = PHINode::Create(llvm::Type::Int64Ty, "", res);
        node->addIncoming(mvm::jit::constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val, 
                            mvm::jit::constantMaxLongFloat,
                            "", currentBlock);

        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMaxLong, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val, 
                            mvm::jit::constantMinLongFloat, "", currentBlock);
        
        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int64Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case F2D :
        push(new FPExtInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             AssessorDesc::dDouble);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      
      case D2I : {
        pop(); // remove the 0 on the stack
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("D2I");
        PHINode* node = PHINode::Create(llvm::Type::Int32Ty, "", res);
        node->addIncoming(mvm::jit::constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val,
                            mvm::jit::constantMaxIntDouble,
                            "", currentBlock);

        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMaxInt, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val,
                            mvm::jit::constantMinIntDouble,
                            "", currentBlock);
        
        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int32Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, AssessorDesc::dInt);

        break;
      }

      case D2L : {
        pop(); // remove the 0 on the stack
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("D2L");
        PHINode* node = PHINode::Create(llvm::Type::Int64Ty, "", res);
        node->addIncoming(mvm::jit::constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val,
                            mvm::jit::constantMaxLongDouble,
                            "", currentBlock);

        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMaxLong, currentBlock);

        currentBlock = cont;

        test = 
          new FCmpInst(FCmpInst::FCMP_OLE, val, mvm::jit::constantMinLongDouble,
                       "", currentBlock);
        
        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(mvm::jit::constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int64Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, AssessorDesc::dLong);
        push(mvm::jit::constantZero, AssessorDesc::dInt);
        break;
      }

      case D2F :
        pop(); // remove the 0 on the stack
        push(new FPTruncInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             AssessorDesc::dFloat);
        break;

      case I2B : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int8Ty, "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case I2C : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int16Ty, "", currentBlock);
        }
        push(new ZExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case I2S : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int16Ty, "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case LCMP : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();

        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_EQ, val1, val2, "",
                                         currentBlock);
        
        BasicBlock* cont = createBasicBlock("LCMP");
        BasicBlock* res = createBasicBlock("LCMP");
        PHINode* node = PHINode::Create(llvm::Type::Int32Ty, "", res);
        node->addIncoming(mvm::jit::constantZero, currentBlock);
        
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;

        test = new ICmpInst(ICmpInst::ICMP_SLT, val1, val2, "", currentBlock);
        node->addIncoming(mvm::jit::constantMinusOne, currentBlock);

        cont = createBasicBlock("LCMP");
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;
        node->addIncoming(mvm::jit::constantOne, currentBlock);
        BranchInst::Create(res, currentBlock);
        currentBlock = res;
        
        push(node, AssessorDesc::dInt);
        break;
      }

      case FCMPL : {
        llvm::Value* val2 = pop();
        llvm::Value* val1 = pop();
        compareFP(val1, val2, Type::FloatTy, false);
        break;
      }

      case FCMPG : {
        llvm::Value* val2 = pop();
        llvm::Value* val1 = pop();
        compareFP(val1, val2, Type::FloatTy, true);
        break;
      }

      case DCMPL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        
        compareFP(val1, val2, Type::DoubleTy, false);
        break;
      }

      case DCMPG : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        
        compareFP(val1, val2, Type::DoubleTy, false);
        break;
      }

      case IFEQ : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;

        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_EQ, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFEQ");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IFNE : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;
        
        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_NE, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFNE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IFLT : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;
        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SLT, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFLT");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IFGE : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;
        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SGE, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFGE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IFGT : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;
        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SGT, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFGT");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IFLE : {
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        const AssessorDesc* ass = topFunc();
        uint8 id = ass->numId;
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
        llvm::Value* val = LAI.llvmNullConstant;
        Value* op = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SLE, op, val, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFLE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPEQ : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_EQ, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPEQ");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPNE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_NE, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPNE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPLT : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SLT, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_IFCMPLT");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }
        
      case IF_ICMPGE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SGE, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPGE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPGT : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SGT, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPGT");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }
      
      case IF_ICMPLE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_SLE, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPLE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ACMPEQ : {
        Value *val2 = pop();
        Value *val1 = pop();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_EQ, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ACMPEQ");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case IF_ACMPNE : {
        Value *val2 = pop();
        Value *val1 = pop();
        uint32 tmp = i;
        BasicBlock* ifTrue = opcodeInfos[tmp + readS2(bytecodes, i)].newBlock;
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_NE, val1, val2, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IF_ACMPNE");
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }

      case GOTO : {
        uint32 tmp = i;
        branch(opcodeInfos[tmp + readS2(bytecodes, i)].newBlock,
               currentBlock);
        break;
      }
      
      case JSR : {
        uint32 tmp = i;
        Value* expr = ConstantExpr::getIntToPtr(
                                    ConstantInt::get(Type::Int64Ty,
                                                     uint64_t (jsrIndex++)),
                                    JnjvmModule::JavaObjectType);

        new StoreInst(expr, supplLocal, false, currentBlock);
        BranchInst::Create(opcodeInfos[tmp + readS2(bytecodes, i)].newBlock,
                       currentBlock);
        break;
      }

      case RET : {
        uint8 local = readU1(bytecodes, i);
        Value* _val = new LoadInst(objectLocals[local], "", currentBlock);
        Value* val = new PtrToIntInst(_val, Type::Int32Ty, "", currentBlock);
        SwitchInst* inst = SwitchInst::Create(val, jsrs[0], jsrs.size(),
                                          currentBlock);
        
        uint32 index = 0;
        for (std::vector<BasicBlock*>::iterator i = jsrs.begin(), 
            e = jsrs.end(); i!= e; ++i, ++index) {
          inst->addCase(ConstantInt::get(Type::Int32Ty, index), *i);
        }

        break;
      }

      case TABLESWITCH : {
        uint32 tmp = i;
        uint32 reste = (i + 1) & 3;
        uint32 filled = reste ?  (4 - reste) : 0;
        i += filled;
        BasicBlock* def = opcodeInfos[tmp + readU4(bytecodes, i)].newBlock;

        uint32 low = readU4(bytecodes, i);
        uint32 high = readU4(bytecodes, i) + 1;
        
        Value* index = pop(); 
        
        const llvm::Type* type = index->getType();
        for (uint32 cur = low; cur < high; ++cur) {
          Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ,
                                    ConstantInt::get(type, cur), index,
                                    "", currentBlock);
          BasicBlock* falseBlock = createBasicBlock("continue tableswitch");
          branch(cmp, opcodeInfos[tmp + readU4(bytecodes, i)].newBlock,
                 falseBlock, currentBlock);
          currentBlock = falseBlock;
        }
       
        
        branch(def, currentBlock);
        i = tmp + 12 + filled + ((high - low) << 2); 

        break;
      }

      case LOOKUPSWITCH : {
        uint32 tmp = i;
        uint32 filled = (3 - i) & 3;
        i += filled;
        BasicBlock* def = opcodeInfos[tmp + readU4(bytecodes, i)].newBlock;
        uint32 nbs = readU4(bytecodes, i);
        
        const AssessorDesc* ass = topFunc();
        Value* key = pop();
        if (ass == AssessorDesc::dShort || ass == AssessorDesc::dByte) {
          key = new SExtInst(key, Type::Int32Ty, "", currentBlock);
        } else if (ass == AssessorDesc::dChar || ass == AssessorDesc::dBool) {
          key = new ZExtInst(key, Type::Int32Ty, "", currentBlock);
        }
        for (uint32 cur = 0; cur < nbs; ++cur) {
          Value* val = ConstantInt::get(Type::Int32Ty, readU4(bytecodes, i));
          Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, val, key, "", currentBlock);
          BasicBlock* falseBlock = createBasicBlock("continue lookupswitch");
          branch(cmp, opcodeInfos[tmp + readU4(bytecodes, i)].newBlock,
                 falseBlock, currentBlock);
          currentBlock = falseBlock;
        }
        branch(def, currentBlock);
        i = tmp + 8 + filled + (nbs << 3);
        break;
      }
      case IRETURN : {
        const AssessorDesc* ass = topFunc();
        Value* val = pop();
        assert(val->getType()->isInteger());
        convertValue(val, returnType, currentBlock, 
                     ass == AssessorDesc::dChar || ass == AssessorDesc::dBool);
        endNode->addIncoming(val, currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        break;
      }
      case LRETURN :
        pop(); // remove the 0 on the stack
        endNode->addIncoming(pop(), currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        break;

      case FRETURN :
        endNode->addIncoming(pop(), currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        break;

      case DRETURN :
        pop(); // remove the 0 on the stack
        endNode->addIncoming(pop(), currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        break;
      
      case ARETURN :
        endNode->addIncoming(pop(), currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        break;
      
      case RETURN :
        BranchInst::Create(endBlock, currentBlock);
        break;

      case GETSTATIC : {
        uint16 index = readU2(bytecodes, i);
        getStaticField(index);
        break;
      }

      case PUTSTATIC : {
        uint16 index = readU2(bytecodes, i);
        setStaticField(index);
        break;
      }

      case GETFIELD : {
        uint16 index = readU2(bytecodes, i);
        getVirtualField(index);
        break;
      }

      case PUTFIELD : {
        uint16 index = readU2(bytecodes, i);
        setVirtualField(index);
        break;
      }

      case INVOKEVIRTUAL : {
        uint16 index = readU2(bytecodes, i);
        invokeVirtual(index);
        break;
      }

      case INVOKESPECIAL : {
        uint16 index = readU2(bytecodes, i);
        invokeSpecial(index);
        break;
      }

      case INVOKESTATIC : {
        uint16 index = readU2(bytecodes, i);
        invokeStatic(index);
        break;
      }

      case INVOKEINTERFACE : {
        uint16 index = readU2(bytecodes, i);
        invokeInterfaceOrVirtual(index);
        i += 2;
        break;
      }

      case NEW : {
        uint16 index = readU2(bytecodes, i);
        invokeNew(index);
        break;
      }

      case NEWARRAY :
      case ANEWARRAY : {
        
        ClassArray* dcl = 0;
        ConstantInt* sizeElement = 0;
        GlobalVariable* TheVT = 0;
        Jnjvm* vm = compilingClass->isolate;

        if (bytecodes[i] == NEWARRAY) {
          uint8 id = bytecodes[++i];
          AssessorDesc* ass = AssessorDesc::arrayType(id);
          dcl = ass->arrayClass;
          TheVT = JnjvmModule::JavaObjectVirtualTableGV;
          LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[ass->numId];
          sizeElement = LAI.sizeInBytesConstant;
        } else {
          uint16 index = readU2(bytecodes, i);
          const UTF8* className = 
            compilingClass->ctpInfo->resolveClassName(index);
        
          const UTF8* arrayName = 
            AssessorDesc::constructArrayName(vm, 0, 1, className);
        
          dcl = vm->constructArray(arrayName, compilingClass->classLoader);
          TheVT = JnjvmModule::ArrayObjectVirtualTableGV;
          sizeElement = mvm::jit::constantPtrSize;
        }
        
        LLVMCommonClassInfo* LCI = module->getClassInfo(dcl);
        llvm::Value* valCl = LCI->getVar(this);
        
        llvm::Value* arg1 = popAsInt();

        Value* cmp = new ICmpInst(ICmpInst::ICMP_SLT, arg1,
                                  mvm::jit::constantZero, "", currentBlock);

        BasicBlock* BB1 = createBasicBlock("");
        BasicBlock* BB2 = createBasicBlock("");

        BranchInst::Create(BB1, BB2, cmp, currentBlock);
        currentBlock = BB1;
        std::vector<Value*> exArgs;
        exArgs.push_back(arg1);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(JnjvmModule::NegativeArraySizeExceptionFunction, 
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", currentBlock);
        } else {
          CallInst::Create(JnjvmModule::NegativeArraySizeExceptionFunction,
                           exArgs.begin(), exArgs.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        currentBlock = BB2;
        
        cmp = new ICmpInst(ICmpInst::ICMP_SGT, arg1,
                           JnjvmModule::MaxArraySizeConstant,
                           "", currentBlock);

        BB1 = createBasicBlock("");
        BB2 = createBasicBlock("");

        BranchInst::Create(BB1, BB2, cmp, currentBlock);
        currentBlock = BB1;
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(JnjvmModule::OutOfMemoryErrorFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", currentBlock);
        } else {
          CallInst::Create(JnjvmModule::OutOfMemoryErrorFunction,
                           exArgs.begin(), exArgs.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        currentBlock = BB2;
        
        Value* mult = BinaryOperator::createMul(arg1, sizeElement, "",
                                                currentBlock);
        Value* size =
          BinaryOperator::createAdd(JnjvmModule::JavaObjectSizeConstant, mult,
                                    "", currentBlock);
        std::vector<Value*> args;
        args.push_back(size);
        args.push_back(new LoadInst(TheVT, "", currentBlock));
#ifdef MULTIPLE_GC
        args.push_back(CallInst::Create(JnjvmModule::GetCollectorFunction,
                                        isolateLocal, "", currentBlock));
#endif
        Value* res = invoke(JnjvmModule::JavaObjectAllocateFunction, args, "",
                            currentBlock);
        Value* cast = new BitCastInst(res, JnjvmModule::JavaArrayType, "",
                                      currentBlock);

        // Set the size
        std::vector<Value*> gep4;
        gep4.push_back(mvm::jit::constantZero);
        gep4.push_back(JnjvmModule::JavaArraySizeOffsetConstant);
        Value* GEP = GetElementPtrInst::Create(cast, gep4.begin(), gep4.end(),
                                               "", currentBlock);
        new StoreInst(arg1, GEP, currentBlock);
        
        // Set the class
        std::vector<Value*> gep;
        gep.push_back(mvm::jit::constantZero);
        gep.push_back(JnjvmModule::JavaObjectClassOffsetConstant);
        GEP = GetElementPtrInst::Create(res, gep.begin(), gep.end(), "",
                                        currentBlock);
        new StoreInst(valCl, GEP, currentBlock);
        
        push(res, AssessorDesc::dRef);

        break;
      }

      case ARRAYLENGTH : {
        Value* val = pop();
        JITVerifyNull(val);
        push(arraySize(val), AssessorDesc::dInt);
        break;
      }

      case ATHROW : {
        llvm::Value* arg = pop();
        std::vector<Value*> args;
        args.push_back(arg);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(JnjvmModule::ThrowExceptionFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, args.begin(), args.end(),
                             "", currentBlock);
        } else {
          CallInst::Create(JnjvmModule::ThrowExceptionFunction, args.begin(),
                           args.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        break;
      }

      case CHECKCAST : {
        uint16 index = readU2(bytecodes, i);
        CommonClass* dcl =
          compilingClass->ctpInfo->getMethodClassIfLoaded(index);
        
        Value* obj = top();

        Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj,
                                  JnjvmModule::JavaObjectNullConstant,
                                  "", currentBlock);
        
        BasicBlock* ifTrue = createBasicBlock("null checkcast");
        BasicBlock* ifFalse = createBasicBlock("non null checkcast");

        BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
        currentBlock = ifFalse;
        Value* clVar = 0;
        if (dcl) {
          LLVMCommonClassInfo* LCI = module->getClassInfo(dcl);
          clVar = LCI->getVar(this);
        } else {
          clVar = getResolvedClass(index, false);
        }
        std::vector<Value*> args;
        args.push_back(obj);
        args.push_back(clVar);
        Value* call = CallInst::Create(JnjvmModule::InstanceOfFunction,
                                       args.begin(), args.end(),
                                       "", currentBlock);
        
        BasicBlock* ex = createBasicBlock("false checkcast");
        BranchInst::Create(ifTrue, ex, call, currentBlock);

        std::vector<Value*> exArgs;
        exArgs.push_back(obj);
        exArgs.push_back(clVar);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(JnjvmModule::ClassCastExceptionFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", ex);
        } else {
          CallInst::Create(JnjvmModule::ClassCastExceptionFunction,
                           exArgs.begin(), exArgs.end(), "", ex);
          new UnreachableInst(ex);
        }
        
        currentBlock = ifTrue;
        break;
      }

      case INSTANCEOF : {
        uint16 index = readU2(bytecodes, i);
        CommonClass* dcl =
          compilingClass->ctpInfo->getMethodClassIfLoaded(index);
        
        Value* clVar = 0;
        if (dcl) {
          LLVMCommonClassInfo* LCI = module->getClassInfo(dcl);
          clVar = LCI->getVar(this);
        } else {
          clVar = getResolvedClass(index, false);
        }
        std::vector<Value*> args;
        args.push_back(pop());
        args.push_back(clVar);
        Value* val = CallInst::Create(JnjvmModule::InstanceOfFunction,
                                      args.begin(), args.end(), "",
                                      currentBlock);
        push(new ZExtInst(val, Type::Int32Ty, "", currentBlock),
             AssessorDesc::dInt);
        break;
      }

      case MONITORENTER : {
        Value* obj = pop();
#ifdef SERVICE_VM
        if (ServiceDomain::isLockableDomain(compilingClass->isolate))
          invoke(JnjvmModule::AquireObjectInSharedDomainFunction, obj, "",
                 currentBlock); 
        else
          invoke(JnjvmModule::AquireObjectFunction, obj, "",
                 currentBlock); 
#else
        JITVerifyNull(obj);
        monitorEnter(obj);
#endif
        break;
      }

      case MONITOREXIT : {
        Value* obj = pop();
#ifdef SERVICE_VM
        if (ServiceDomain::isLockableDomain(compilingClass->isolate))
          invoke(JnjvmModule::ReleaseObjectInSharedDomainFunction, obj, "",
                 currentBlock); 
        else
          invoke(JnjvmModule::ReleaseObjectFunction, obj, "",
                 currentBlock); 
#else
        JITVerifyNull(obj);
        monitorExit(obj);
#endif
        break;
      }

      case MULTIANEWARRAY : {
        Jnjvm* vm = compilingClass->isolate;
        uint16 index = readU2(bytecodes, i);
        uint8 dim = readU1(bytecodes, i);
        
        const UTF8* className = 
          compilingClass->ctpInfo->resolveClassName(index);

        ClassArray* dcl = 
          vm->constructArray(className, compilingClass->classLoader);
        
        compilingClass->ctpInfo->loadClass(index);
        
        LLVMCommonClassInfo* LCI = module->getClassInfo(dcl);
        Value* valCl = LCI->getVar(this);
        Value* args[dim + 2];
        args[0] = valCl;
        args[1] = ConstantInt::get(Type::Int32Ty, dim);

        for (int cur = dim + 1; cur >= 2; --cur)
          args[cur] = pop();
        
        std::vector<Value*> Args;
        for (sint32 v = 0; v < dim + 2; ++v) {
          Args.push_back(args[v]);
        }
#ifdef MULTIPLE_VM
        Args.push_back(isolateLocal);
#endif
        push(invoke(JnjvmModule::MultiCallNewFunction, Args, "", currentBlock),
             AssessorDesc::dRef);
        break;
      }

      case WIDE :
        wide = true;
        break;

      case IFNULL : {
        uint32 tmp = i;
        const AssessorDesc* ass = topFunc();
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[ass->numId];
        llvm::Value* nil = LAI.llvmNullConstant;
        llvm::Value* val = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_EQ, val, nil, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("true IFNULL");
        BasicBlock* ifTrue = opcodeInfos[readS2(bytecodes, i) + tmp].newBlock;
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }
      
      case IFNONNULL : {
        uint32 tmp = i;
        const AssessorDesc* ass = topFunc();
        LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[ass->numId];
        llvm::Value* nil = LAI.llvmNullConstant;
        llvm::Value* val = pop();
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_NE, val, nil, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFNONNULL");
        BasicBlock* ifTrue = opcodeInfos[readS2(bytecodes, i) + tmp].newBlock;
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }
      
      default :
        JavaThread::get()->isolate->unknownError("unknown bytecode");

    } 
  }
}

void JavaJIT::exploreOpcodes(uint8* bytecodes, uint32 codeLength) {
  wide = false;
  for(uint32 i = 0; i < codeLength; ++i) {
    
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "\t[at %5d] %-5d ", i,
                bytecodes[i]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "exploring ");
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n");
    
    switch (bytecodes[i]) {
      
      case ACONST_NULL : 
      case ICONST_M1 :
      case ICONST_0 :
      case ICONST_1 :
      case ICONST_2 :
      case ICONST_3 :
      case ICONST_4 :
      case ICONST_5 :
      case LCONST_0 :
      case LCONST_1 :
      case FCONST_0 :
      case FCONST_1 : 
      case FCONST_2 :
      case DCONST_0 :
      case DCONST_1 : break;

      case BIPUSH : ++i; break;
      
      case SIPUSH : i += 2; break;
      
      case LDC : ++i; break;

      case LDC_W : 
      case LDC2_W : i += 2; break;

      case ILOAD :
      case LLOAD :
      case FLOAD :
      case DLOAD :
      case ALOAD :
        i += WCALC(1);
        break;
      
      case ILOAD_0 :
      case ILOAD_1 :
      case ILOAD_2 :
      case ILOAD_3 :
      case LLOAD_0 :
      case LLOAD_1 :
      case LLOAD_2 :
      case LLOAD_3 :
      case FLOAD_0 :
      case FLOAD_1 :
      case FLOAD_2 :
      case FLOAD_3 :
      case DLOAD_0 :
      case DLOAD_1 :
      case DLOAD_2 :
      case DLOAD_3 :
      case ALOAD_0 :
      case ALOAD_1 :
      case ALOAD_2 :
      case ALOAD_3 :
      case IALOAD :
      case LALOAD :
      case FALOAD :
      case DALOAD :
      case AALOAD :
      case BALOAD :
      case CALOAD :
      case SALOAD : break;

      case ISTORE :
      case LSTORE :
      case FSTORE :
      case DSTORE :
      case ASTORE :
        i += WCALC(1);
        break;
      
      case ISTORE_0 :
      case ISTORE_1 :
      case ISTORE_2 :
      case ISTORE_3 :
      case LSTORE_0 :
      case LSTORE_1 :
      case LSTORE_2 :
      case LSTORE_3 :
      case FSTORE_0 :
      case FSTORE_1 :
      case FSTORE_2 :
      case FSTORE_3 :
      case DSTORE_0 :
      case DSTORE_1 :
      case DSTORE_2 :
      case DSTORE_3 :
      case ASTORE_0 :
      case ASTORE_1 :
      case ASTORE_2 :
      case ASTORE_3 :
      case IASTORE :
      case LASTORE :
      case FASTORE :
      case DASTORE :
      case AASTORE :
      case BASTORE :
      case CASTORE :
      case SASTORE :
      case POP :
      case POP2 :
      case DUP :
      case DUP_X1 :
      case DUP_X2 :
      case DUP2 :
      case DUP2_X1 :
      case DUP2_X2 :
      case SWAP :
      case IADD :
      case LADD :
      case FADD :
      case DADD :
      case ISUB :
      case LSUB :
      case FSUB :
      case DSUB :
      case IMUL :
      case LMUL :
      case FMUL :
      case DMUL :
      case IDIV :
      case LDIV :
      case FDIV :
      case DDIV :
      case IREM :
      case LREM :
      case FREM :
      case DREM :
      case INEG :
      case LNEG :
      case FNEG :
      case DNEG :
      case ISHL :
      case LSHL :
      case ISHR :
      case LSHR :
      case IUSHR :
      case LUSHR :
      case IAND :
      case LAND :
      case IOR :
      case LOR :
      case IXOR :
      case LXOR : break;

      case IINC :
        i += WCALC(2);
        break;
      
      case I2L :
      case I2F :
      case I2D :
      case L2I :
      case L2F :
      case L2D :
      case F2I :
      case F2L :
      case F2D :
      case D2I :
      case D2L :
      case D2F :
      case I2B :
      case I2C :
      case I2S :
      case LCMP :
      case FCMPL :
      case FCMPG :
      case DCMPL :
      case DCMPG : break;

      case IFEQ :
      case IFNE :
      case IFLT :
      case IFGE :
      case IFGT :
      case IFLE :
      case IF_ICMPEQ :
      case IF_ICMPNE :
      case IF_ICMPLT :
      case IF_ICMPGE :
      case IF_ICMPGT :
      case IF_ICMPLE :
      case IF_ACMPEQ :
      case IF_ACMPNE :
      case GOTO : {
        uint32 tmp = i;
        uint16 index = tmp + readU2(bytecodes, i);
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("GOTO or IF*");
        break;
      }
      
      case JSR : {
        uint32 tmp = i;
        uint16 index = tmp + readU2(bytecodes, i);
        if (!(opcodeInfos[index].newBlock)) {
          BasicBlock* block = createBasicBlock("JSR");
          opcodeInfos[index].newBlock = block;
        }
        if (!(opcodeInfos[tmp + 3].newBlock)) {
          BasicBlock* block = createBasicBlock("JSR2");
          jsrs.push_back(block);
          opcodeInfos[tmp + 3].newBlock = block;
        } else {
          jsrs.push_back(opcodeInfos[tmp + 3].newBlock);
        }
        opcodeInfos[index].reqSuppl = true;
        break;
      }

      case RET : ++i; break;

      case TABLESWITCH : {
        uint32 tmp = i;
        uint32 reste = (i + 1) & 3;
        uint32 filled = reste ? (4 - reste) : 0; 
        i += filled;
        uint32 index = tmp + readU4(bytecodes, i);
        if (!(opcodeInfos[index].newBlock)) {
          BasicBlock* block = createBasicBlock("tableswitch");
          opcodeInfos[index].newBlock = block;
        }
        uint32 low = readU4(bytecodes, i);
        uint32 high = readU4(bytecodes, i) + 1;
        uint32 depl = high - low;
        for (uint32 cur = 0; cur < depl; ++cur) {
          uint32 index2 = tmp + readU4(bytecodes, i);
          if (!(opcodeInfos[index2].newBlock)) {
            BasicBlock* block = createBasicBlock("tableswitch");
            opcodeInfos[index2].newBlock = block;
          }
        }
        i = tmp + 12 + filled + (depl << 2);
        break;
      }

      case LOOKUPSWITCH : {
        uint32 tmp = i;
        uint32 filled = (3 - i) & 3;
        i += filled;
        uint32 index = tmp + readU4(bytecodes, i);
        if (!(opcodeInfos[index].newBlock)) {
          BasicBlock* block = createBasicBlock("tableswitch");
          opcodeInfos[index].newBlock = block;
        }
        uint32 nbs = readU4(bytecodes, i);
        for (uint32 cur = 0; cur < nbs; ++cur) {
          i += 4;
          uint32 index2 = tmp + readU4(bytecodes, i);
          if (!(opcodeInfos[index2].newBlock)) {
            BasicBlock* block = createBasicBlock("tableswitch");
            opcodeInfos[index2].newBlock = block;
          }
        }
        
        i = tmp + 8 + filled + (nbs << 3);
        break;
      }

      case IRETURN :
      case LRETURN :
      case FRETURN :
      case DRETURN :
      case ARETURN :
      case RETURN : break;
      
      case GETSTATIC :
      case PUTSTATIC :
      case GETFIELD :
      case PUTFIELD :
      case INVOKEVIRTUAL :
      case INVOKESPECIAL :
      case INVOKESTATIC :
        i += 2;
        break;
      
      case INVOKEINTERFACE :
        i += 4;
        break;

      case NEW :
        i += 2;
        break;

      case NEWARRAY :
        ++i;
        break;
      
      case ANEWARRAY :
        i += 2;
        break;

      case ARRAYLENGTH :
      case ATHROW : break;

      case CHECKCAST :
        i += 2;
        break;

      case INSTANCEOF :
        i += 2;
        break;
      
      case MONITORENTER :
        break;

      case MONITOREXIT :
        break;
      
      case MULTIANEWARRAY :
        i += 3;
        break;

      case WIDE :
        wide = true;
        break;

      case IFNULL :
      case IFNONNULL : {
        uint32 tmp = i;
        uint16 index = tmp + readU2(bytecodes, i);
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("true IF*NULL");
        break;
      }


      default :
        JavaThread::get()->isolate->unknownError("unknown bytecode");
    }
  }
}
