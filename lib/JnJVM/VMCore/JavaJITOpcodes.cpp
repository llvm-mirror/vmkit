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

uint8 arrayType(unsigned int t) {
  if (t == JavaArray::T_CHAR) {
    return I_CHAR;
  } else if (t == JavaArray::T_BOOLEAN) {
    return I_BOOL;
  } else if (t == JavaArray::T_INT) {
    return I_INT;
  } else if (t == JavaArray::T_SHORT) {
    return I_SHORT;
  } else if (t == JavaArray::T_BYTE) {
    return I_BYTE;
  } else if (t == JavaArray::T_FLOAT) {
    return I_FLOAT;
  } else if (t == JavaArray::T_LONG) {
    return I_LONG;
  } else if (t == JavaArray::T_DOUBLE) {
    return I_DOUBLE;
  } else {
    JavaThread::get()->getJVM()->unknownError("unknown array type %d\n", t);
    return 0;
  }
}

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
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "compiling ", 0);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]], 0);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n", 0);
    
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
    args.push_back(ConstantExpr::getIntToPtr(
          ConstantInt::get(Type::Int64Ty, (int64_t)OpcodeNames[bytecodes[i]]),
          module->ptrType));

    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)i));
    
    args.push_back(ConstantExpr::getIntToPtr(
          ConstantInt::get(Type::Int64Ty, (int64_t)compilingMethod),
          module->ptrType));
    
    
    CallInst::Create(module->PrintExecutionFunction, args.begin(),
                     args.end(), "", currentBlock);
    }
#endif
    
    // If we're an exception handler and no one has pushed our exception
    // variable on stack, then do it now.
    if (opinfo->reqSuppl && stack.size() != 1) {
      push(new LoadInst(supplLocal, "", currentBlock), false);
    }

    switch (bytecodes[i]) {
      
      case ACONST_NULL : 
        push(module->JavaObjectNullConstant, false);
        break;

      case ICONST_M1 :
        push(module->constantMinusOne, false);
        break;

      case ICONST_0 :
        push(module->constantZero, false);
        break;

      case ICONST_1 :
        push(module->constantOne, false);
        break;

      case ICONST_2 :
        push(module->constantTwo, false);
        break;

      case ICONST_3 :
        push(module->constantThree, false);
        break;

      case ICONST_4 :
        push(module->constantFour, false);
        break;

      case ICONST_5 :
        push(module->constantFive, false);
        break;

      case LCONST_0 :
        push(module->constantLongZero, false);
        push(module->constantZero, false);
        break;

      case LCONST_1 :
        push(module->constantLongOne, false);
        push(module->constantZero, false);
        break;

      case FCONST_0 :
        push(module->constantFloatZero, false);
        break;

      case FCONST_1 :
        push(module->constantFloatOne, false);
        break;
      
      case FCONST_2 :
        push(module->constantFloatTwo, false);
        break;
      
      case DCONST_0 :
        push(module->constantDoubleZero, false);
        push(module->constantZero, false);
        break;
      
      case DCONST_1 :
        push(module->constantDoubleOne, false);
        push(module->constantZero, false);
        break;

      case BIPUSH : 
        push(ConstantExpr::getSExt(ConstantInt::get(Type::Int8Ty,
                                                    bytecodes[++i]),
                                   Type::Int32Ty), false);
        break;

      case SIPUSH :
        push(ConstantExpr::getSExt(ConstantInt::get(Type::Int16Ty,
                                                    readS2(bytecodes, i)),
                                   Type::Int32Ty), false);
        break;

      case LDC :
        _ldc(bytecodes[++i]);
        break;

      case LDC_W :
        _ldc(readS2(bytecodes, i));
        break;

      case LDC2_W :
        _ldc(readS2(bytecodes, i));
        push(module->constantZero, false);
        break;

      case ILOAD :
        push(new LoadInst(intLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), false);
        break;

      case LLOAD :
        push(new LoadInst(longLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), false);
        push(module->constantZero, false);
        break;

      case FLOAD :
        push(new LoadInst(floatLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), false);
        break;

      case DLOAD :
        push(new LoadInst(doubleLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), false);
        push(module->constantZero, false);
        break;

      case ALOAD :
        push(new LoadInst(objectLocals[WREAD_U1(bytecodes, false, i)], "",
                          currentBlock), false);
        break;
      
      case ILOAD_0 :
        push(new LoadInst(intLocals[0], "", currentBlock), false);
        break;
      
      case ILOAD_1 :
        push(new LoadInst(intLocals[1], "", currentBlock), false);
        break;

      case ILOAD_2 :
        push(new LoadInst(intLocals[2], "", currentBlock), false);
        break;

      case ILOAD_3 :
        push(new LoadInst(intLocals[3], "", currentBlock), false);
        break;
      
      case LLOAD_0 :
        push(new LoadInst(longLocals[0], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;

      case LLOAD_1 :
        push(new LoadInst(longLocals[1], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case LLOAD_2 :
        push(new LoadInst(longLocals[2], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case LLOAD_3 :
        push(new LoadInst(longLocals[3], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case FLOAD_0 :
        push(new LoadInst(floatLocals[0], "", currentBlock),
             false);
        break;
      
      case FLOAD_1 :
        push(new LoadInst(floatLocals[1], "", currentBlock),
             false);
        break;

      case FLOAD_2 :
        push(new LoadInst(floatLocals[2], "", currentBlock),
             false);
        break;

      case FLOAD_3 :
        push(new LoadInst(floatLocals[3], "", currentBlock),
             false);
        break;
      
      case DLOAD_0 :
        push(new LoadInst(doubleLocals[0], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;

      case DLOAD_1 :
        push(new LoadInst(doubleLocals[1], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case DLOAD_2 :
        push(new LoadInst(doubleLocals[2], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case DLOAD_3 :
        push(new LoadInst(doubleLocals[3], "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case ALOAD_0 :
        push(new LoadInst(objectLocals[0], "", currentBlock),
             false);
        break;
      
      case ALOAD_1 :
        push(new LoadInst(objectLocals[1], "", currentBlock),
             false);
        break;

      case ALOAD_2 :
        push(new LoadInst(objectLocals[2], "", currentBlock),
             false);
        break;

      case ALOAD_3 :
        push(new LoadInst(objectLocals[3], "", currentBlock),
             false);
        break;
      
      case IALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, 
                                         module->JavaArraySInt32Type);
        push(new LoadInst(ptr, "", currentBlock), false);
        break;
      }

      case LALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayLongType);
        push(new LoadInst(ptr, "", currentBlock), false);
        push(module->constantZero, false);
        break;
      }

      case FALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayFloatType);
        push(new LoadInst(ptr, "", currentBlock), false);
        break;
      }

      case DALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayDoubleType);
        push(new LoadInst(ptr, "", currentBlock), false);
        push(module->constantZero, false);
        break;
      }

      case AALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayObjectType);
        push(new LoadInst(ptr, "", currentBlock), false);
        break;
      }

      case BALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArraySInt8Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::Int32Ty, "", currentBlock),
             false);
        break;
      }

      case CALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayUInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new ZExtInst(val, Type::Int32Ty, "", currentBlock),
             false);
        break;
      }

      case SALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArraySInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::Int32Ty, "", currentBlock),
             false);
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
                                         module->JavaArraySInt32Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case LASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayLongType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case FASTORE : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayFloatType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case DASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayDoubleType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case AASTORE : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayObjectType);
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
                                         module->JavaArraySInt8Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case CASTORE : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::Int32Ty) {
          val = new TruncInst(val, Type::Int16Ty, "", currentBlock);
        } else if (type == Type::Int8Ty) {
          val = new ZExtInst(val, Type::Int16Ty, "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArrayUInt16Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case SASTORE : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::Int32Ty) {
          val = new TruncInst(val, Type::Int16Ty, "", currentBlock);
        } else if (type == Type::Int8Ty) {
          val = new SExtInst(val, Type::Int16Ty, "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         module->JavaArraySInt16Type);
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
        std::pair<Value*, bool> one = popPair();
        std::pair<Value*, bool> two = popPair();
        push(one);
        push(two);
        push(one);
        break;
      }

      case DUP_X2 : {
        std::pair<Value*, bool> one = popPair();
        std::pair<Value*, bool> two = popPair();
        std::pair<Value*, bool> three = popPair();
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
        std::pair<Value*, bool> one = popPair();
        std::pair<Value*, bool> two = popPair();
        std::pair<Value*, bool> three = popPair();

        push(two);
        push(one);

        push(three);
        push(two);
        push(one);

        break;
      }

      case DUP2_X2 : {
        std::pair<Value*, bool> one = popPair();
        std::pair<Value*, bool> two = popPair();
        std::pair<Value*, bool> three = popPair();
        std::pair<Value*, bool> four = popPair();

        push(two);
        push(one);
        
        push(four);
        push(three);
        push(two);
        push(one);

        break;
      }

      case SWAP : {
        std::pair<Value*, bool> one = popPair();
        std::pair<Value*, bool> two = popPair();
        push(one);
        push(two);
        break;
      }

      case IADD : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateAdd(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LADD : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateAdd(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case FADD : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateAdd(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DADD : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateAdd(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case ISUB : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateSub(val1, val2, "", currentBlock),
             false);
        break;
      }
      case LSUB : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateSub(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case FSUB : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateSub(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DSUB : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateSub(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IMUL : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateMul(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LMUL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateMul(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case FMUL : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateMul(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DMUL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateMul(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IDIV : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateSDiv(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LDIV : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateSDiv(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case FDIV : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateFDiv(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DDIV : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateFDiv(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IREM : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateSRem(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LREM : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateSRem(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case FREM : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateFRem(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DREM : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateFRem(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case INEG :
        push(BinaryOperator::CreateSub(
                              module->constantZero,
                              popAsInt(), "", currentBlock),
             false);
        break;
      
      case LNEG : {
        pop();
        push(BinaryOperator::CreateSub(
                              module->constantLongZero,
                              pop(), "", currentBlock), false);
        push(module->constantZero, false);
        break;
      }

      case FNEG :
        push(BinaryOperator::CreateSub(
                              module->constantFloatMinusZero,
                              pop(), "", currentBlock), false);
        break;
      
      case DNEG : {
        pop();
        push(BinaryOperator::CreateSub(
                              module->constantDoubleMinusZero,
                              pop(), "", currentBlock), false);
        push(module->constantZero, false);
        break;
      }

      case ISHL : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateShl(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LSHL : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateShl(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case ISHR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateAShr(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LSHR : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateAShr(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IUSHR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        Value* mask = ConstantInt::get(Type::Int32Ty, 0x1F);
        val2 = BinaryOperator::CreateAnd(val2, mask, "", currentBlock);
        push(BinaryOperator::CreateLShr(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LUSHR : {
        Value* val2 = new ZExtInst(pop(), Type::Int64Ty, "", currentBlock);
        Value* mask = ConstantInt::get(Type::Int64Ty, 0x3F);
        val2 = BinaryOperator::CreateAnd(val2, mask, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateLShr(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IAND : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateAnd(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LAND : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateAnd(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IOR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateOr(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LOR : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateOr(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IXOR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        push(BinaryOperator::CreateXor(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LXOR : {
        pop();
        Value* val2 = pop();
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateXor(val1, val2, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      }

      case IINC : {
        uint16 idx = WREAD_U1(bytecodes, true, i);
        sint16 val = WREAD_S1(bytecodes, false, i);
        llvm::Value* add = BinaryOperator::CreateAdd(
            new LoadInst(intLocals[idx], "", currentBlock), 
            ConstantInt::get(Type::Int32Ty, val), "",
            currentBlock);
        new StoreInst(add, intLocals[idx], false, currentBlock);
        break;
      }

      case I2L :
        push(new SExtInst(pop(), llvm::Type::Int64Ty, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;

      case I2F :
        push(new SIToFPInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             false);
        break;
        
      case I2D :
        push(new SIToFPInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case L2I :
        pop();
        push(new TruncInst(pop(), llvm::Type::Int32Ty, "", currentBlock),
             false);
        break;
      
      case L2F :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             false);
        break;
      
      case L2D :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case F2I : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("F2I");
        PHINode* node = PHINode::Create(llvm::Type::Int32Ty, "", res);
        node->addIncoming(module->constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val, 
                            module->constantMaxIntFloat,
                            "", currentBlock);

        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMaxInt,
                          currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val,
                            module->constantMinIntFloat,
                            "", currentBlock);
        
        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int32Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;

        push(node, false);
        break;
      }

      case F2L : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("F2L");
        PHINode* node = PHINode::Create(llvm::Type::Int64Ty, "", res);
        node->addIncoming(module->constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val, 
                            module->constantMaxLongFloat,
                            "", currentBlock);

        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMaxLong, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val, 
                            module->constantMinLongFloat, "", currentBlock);
        
        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int64Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, false);
        push(module->constantZero, false);
        break;
      }

      case F2D :
        push(new FPExtInst(pop(), llvm::Type::DoubleTy, "", currentBlock),
             false);
        push(module->constantZero, false);
        break;
      
      case D2I : {
        pop(); // remove the 0 on the stack
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("D2I");
        PHINode* node = PHINode::Create(llvm::Type::Int32Ty, "", res);
        node->addIncoming(module->constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val,
                            module->constantMaxIntDouble,
                            "", currentBlock);

        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMaxInt, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(FCmpInst::FCMP_OLE, val,
                            module->constantMinIntDouble,
                            "", currentBlock);
        
        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int32Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, false);

        break;
      }

      case D2L : {
        pop(); // remove the 0 on the stack
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(FCmpInst::FCMP_ONE, val, val, "",
                                         currentBlock);
        
        BasicBlock* res = createBasicBlock("D2L");
        PHINode* node = PHINode::Create(llvm::Type::Int64Ty, "", res);
        node->addIncoming(module->constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(FCmpInst::FCMP_OGE, val,
                            module->constantMaxLongDouble,
                            "", currentBlock);

        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMaxLong, currentBlock);

        currentBlock = cont;

        test = 
          new FCmpInst(FCmpInst::FCMP_OLE, val, module->constantMinLongDouble,
                       "", currentBlock);
        
        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(module->constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::Int64Ty, "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, false);
        push(module->constantZero, false);
        break;
      }

      case D2F :
        pop(); // remove the 0 on the stack
        push(new FPTruncInst(pop(), llvm::Type::FloatTy, "", currentBlock),
             false);
        break;

      case I2B : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int8Ty, "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             false);
        break;
      }

      case I2C : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int16Ty, "", currentBlock);
        }
        push(new ZExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             false);
        break;
      }

      case I2S : {
        Value* val = pop();
        if (val->getType() == Type::Int32Ty) {
          val = new TruncInst(val, llvm::Type::Int16Ty, "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::Int32Ty, "", currentBlock),
             false);
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
        node->addIncoming(module->constantZero, currentBlock);
        
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;

        test = new ICmpInst(ICmpInst::ICMP_SLT, val1, val2, "", currentBlock);
        node->addIncoming(module->constantMinusOne, currentBlock);

        cont = createBasicBlock("LCMP");
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;
        node->addIncoming(module->constantOne, currentBlock);
        BranchInst::Create(res, currentBlock);
        currentBlock = res;
        
        push(node, false);
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

        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
        
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
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
                                    module->JavaObjectType);
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
        
        bool unsign = topFunc();
        Value* key = pop();
        const Type* type = key->getType();
        if (unsign) {
          key = new ZExtInst(key, Type::Int32Ty, "", currentBlock);
        } else if (type == Type::Int8Ty || type == Type::Int16Ty) {
          key = new SExtInst(key, Type::Int32Ty, "", currentBlock);
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
        bool unsign = topFunc();
        Value* val = pop();
        assert(val->getType()->isInteger());
        convertValue(val, returnType, currentBlock, unsign);
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
        
#ifndef ISOLATE_SHARING
        UserClassArray* dcl = 0;
#endif
        ConstantInt* sizeElement = 0;
        Value* TheVT = 0;
        Value* valCl = 0;

        if (bytecodes[i] == NEWARRAY) {
          uint8 id = bytecodes[++i];
          uint8 charId = arrayType(id);
#ifndef ISOLATE_SHARING
          JnjvmBootstrapLoader* loader = 
            compilingClass->classLoader->bootstrapLoader;
          dcl = loader->getArrayClass(id);
#else
          std::vector<Value*> args;
          args.push_back(isolateLocal);
          args.push_back(ConstantInt::get(Type::Int32Ty, id - 4));
          valCl = CallInst::Create(module->GetJnjvmArrayClassFunction,
                                   args.begin(), args.end(), "", currentBlock);
#endif

          LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[charId];
          sizeElement = LAI.sizeInBytesConstant;
        } else {
          uint16 index = readU2(bytecodes, i);
#ifndef ISOLATE_SHARING
          const UTF8* className = 
            compilingClass->ctpInfo->resolveClassName(index);
        
          JnjvmClassLoader* JCL = compilingClass->classLoader;
          const UTF8* arrayName = JCL->constructArrayName(1, className);
        
          dcl = JCL->constructArray(arrayName);
#else

          valCl = getResolvedClass(index, false);
          valCl = CallInst::Create(module->GetArrayClassFunction, valCl,
                                   "", currentBlock);
#endif
          sizeElement = module->constantPtrSize;
        }
#ifndef ISOLATE_SHARING
        valCl = module->getNativeClass(dcl);
        valCl = new LoadInst(valCl, "", currentBlock);
        TheVT = module->getVirtualTable(dcl);
        TheVT = new LoadInst(TheVT, "", currentBlock);
#endif   
        llvm::Value* arg1 = popAsInt();

        Value* cmp = new ICmpInst(ICmpInst::ICMP_SLT, arg1,
                                  module->constantZero, "", currentBlock);

        BasicBlock* BB1 = createBasicBlock("");
        BasicBlock* BB2 = createBasicBlock("");

        BranchInst::Create(BB1, BB2, cmp, currentBlock);
        currentBlock = BB1;
        std::vector<Value*> exArgs;
        exArgs.push_back(arg1);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(module->NegativeArraySizeExceptionFunction, 
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", currentBlock);
        } else {
          CallInst::Create(module->NegativeArraySizeExceptionFunction,
                           exArgs.begin(), exArgs.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        currentBlock = BB2;
        
        cmp = new ICmpInst(ICmpInst::ICMP_SGT, arg1,
                           module->MaxArraySizeConstant,
                           "", currentBlock);

        BB1 = createBasicBlock("");
        BB2 = createBasicBlock("");

        BranchInst::Create(BB1, BB2, cmp, currentBlock);
        currentBlock = BB1;
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(module->OutOfMemoryErrorFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", currentBlock);
        } else {
          CallInst::Create(module->OutOfMemoryErrorFunction,
                           exArgs.begin(), exArgs.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        currentBlock = BB2;
        
        Value* mult = BinaryOperator::CreateMul(arg1, sizeElement, "",
                                                currentBlock);
        Value* size =
          BinaryOperator::CreateAdd(module->JavaObjectSizeConstant, mult,
                                    "", currentBlock);
        assert(TheVT && "Not VT");
        std::vector<Value*> args;
        args.push_back(size);
        args.push_back(TheVT);
        Value* res = invoke(module->JavaObjectAllocateFunction, args, "",
                            currentBlock);
        Value* cast = new BitCastInst(res, module->JavaArrayType, "",
                                      currentBlock);

        // Set the size
        std::vector<Value*> gep4;
        gep4.push_back(module->constantZero);
        gep4.push_back(module->JavaArraySizeOffsetConstant);
        Value* GEP = GetElementPtrInst::Create(cast, gep4.begin(), gep4.end(),
                                               "", currentBlock);
        new StoreInst(arg1, GEP, currentBlock);
        
        // Set the class
        std::vector<Value*> gep;
        gep.push_back(module->constantZero);
        gep.push_back(module->JavaObjectClassOffsetConstant);
        GEP = GetElementPtrInst::Create(res, gep.begin(), gep.end(), "",
                                        currentBlock);
        new StoreInst(valCl, GEP, currentBlock);
        
        gep.clear();
        gep.push_back(module->constantZero);
        gep.push_back(module->JavaObjectLockOffsetConstant);
        Value* lockPtr = GetElementPtrInst::Create(res, gep.begin(), gep.end(),
                                                   "", currentBlock);
        Value* threadId = CallInst::Create(module->llvm_frameaddress,
                                           module->constantZero, "",
                                           currentBlock);
        threadId = new PtrToIntInst(threadId, module->pointerSizeType, "",
                                    currentBlock);
        threadId = BinaryOperator::CreateAnd(threadId,
                                             module->constantThreadIDMask,
                                             "", currentBlock);
        threadId = new IntToPtrInst(threadId, module->ptrType, "",
                                    currentBlock);

        new StoreInst(threadId, lockPtr, currentBlock);

        push(res, false);

        break;
      }

      case ARRAYLENGTH : {
        Value* val = pop();
        JITVerifyNull(val);
        push(arraySize(val), false);
        break;
      }

      case ATHROW : {
        llvm::Value* arg = pop();
        std::vector<Value*> args;
        args.push_back(arg);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(module->ThrowExceptionFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, args.begin(), args.end(),
                             "", currentBlock);
        } else {
          CallInst::Create(module->ThrowExceptionFunction, args.begin(),
                           args.end(), "", currentBlock);
          new UnreachableInst(currentBlock);
        }
        break;
      }

      case CHECKCAST : {
        uint16 index = readU2(bytecodes, i);
        
        Value* obj = top();

        Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, obj,
                                  module->JavaObjectNullConstant,
                                  "", currentBlock);
        
        BasicBlock* ifTrue = createBasicBlock("null checkcast");
        BasicBlock* ifFalse = createBasicBlock("non null checkcast");

        BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
        currentBlock = ifFalse;
        Value* clVar = 0;
#ifndef ISOLATE_SHARING
        CommonClass* dcl =
          compilingClass->ctpInfo->getMethodClassIfLoaded(index);
        if (dcl) {
          clVar = module->getNativeClass(dcl);
          clVar = new LoadInst(clVar, "", currentBlock);
        } else
#endif
          clVar = getResolvedClass(index, false);
        
        std::vector<Value*> args;
        args.push_back(obj);
        args.push_back(clVar);
        Value* call = CallInst::Create(module->InstanceOfFunction,
                                       args.begin(), args.end(),
                                       "", currentBlock);
        
        BasicBlock* ex = createBasicBlock("false checkcast");
        BranchInst::Create(ifTrue, ex, call, currentBlock);

        std::vector<Value*> exArgs;
        exArgs.push_back(obj);
        exArgs.push_back(clVar);
        if (currentExceptionBlock != endExceptionBlock) {
          InvokeInst::Create(module->ClassCastExceptionFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, exArgs.begin(),
                             exArgs.end(), "", ex);
        } else {
          CallInst::Create(module->ClassCastExceptionFunction,
                           exArgs.begin(), exArgs.end(), "", ex);
          new UnreachableInst(ex);
        }
        
        currentBlock = ifTrue;
        break;
      }

      case INSTANCEOF : {
        uint16 index = readU2(bytecodes, i);
#ifndef ISOLATE_SHARING
        CommonClass* dcl =
          compilingClass->ctpInfo->getMethodClassIfLoaded(index);
        
        Value* clVar = 0;
        if (dcl) {
          clVar = module->getNativeClass(dcl);
          clVar = new LoadInst(clVar, "", currentBlock);
        } else {
          clVar = getResolvedClass(index, false);
        }
#else
        Value* clVar = getResolvedClass(index, false);
#endif
        std::vector<Value*> args;
        args.push_back(pop());
        args.push_back(clVar);
        Value* val = CallInst::Create(module->InstanceOfFunction,
                                      args.begin(), args.end(), "",
                                      currentBlock);
        push(new ZExtInst(val, Type::Int32Ty, "", currentBlock),
             false);
        break;
      }

      case MONITORENTER : {
        Value* obj = pop();
        JITVerifyNull(obj);
        monitorEnter(obj);
        break;
      }

      case MONITOREXIT : {
        Value* obj = pop();
        JITVerifyNull(obj);
        monitorExit(obj);
        break;
      }

      case MULTIANEWARRAY : {
        uint16 index = readU2(bytecodes, i);
        uint8 dim = readU1(bytecodes, i);
        
        
#ifdef ISOLATE_SHARING
        Value* valCl = getResolvedClass(index, true);
#else
        JnjvmClassLoader* JCL = compilingClass->classLoader;
        const UTF8* className = 
          compilingClass->ctpInfo->resolveClassName(index);

        UserClassArray* dcl = JCL->constructArray(className);
        
        compilingClass->ctpInfo->loadClass(index);
        Value* valCl = module->getNativeClass(dcl);
        valCl = new LoadInst(valCl, "", currentBlock);
#endif
        Value** args = (Value**)alloca(sizeof(Value*) * (dim + 2));
        args[0] = valCl;
        args[1] = ConstantInt::get(Type::Int32Ty, dim);

        for (int cur = dim + 1; cur >= 2; --cur)
          args[cur] = pop();
        
        std::vector<Value*> Args;
        for (sint32 v = 0; v < dim + 2; ++v) {
          Args.push_back(args[v]);
        }
        push(invoke(module->MultiCallNewFunction, Args, "", currentBlock),
             false);
        break;
      }

      case WIDE :
        wide = true;
        break;

      case IFNULL : {
        uint32 tmp = i;
        llvm::Value* val = pop();
        Constant* nil = Constant::getNullValue(val->getType());
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
        llvm::Value* val = pop();
        Constant* nil = Constant::getNullValue(val->getType());
        llvm::Value* test = new ICmpInst(ICmpInst::ICMP_NE, val, nil, "",
                                         currentBlock);
        BasicBlock* ifFalse = createBasicBlock("false IFNONNULL");
        BasicBlock* ifTrue = opcodeInfos[readS2(bytecodes, i) + tmp].newBlock;
        branch(test, ifTrue, ifFalse, currentBlock);
        currentBlock = ifFalse;
        break;
      }
      
      default :
        JavaThread::get()->getJVM()->unknownError("unknown bytecode");

    } 
  }
}

void JavaJIT::exploreOpcodes(uint8* bytecodes, uint32 codeLength) {
  wide = false;
  for(uint32 i = 0; i < codeLength; ++i) {
    
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "\t[at %5d] %-5d ", i,
                bytecodes[i]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "exploring ",0);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecodes[i]], 0);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n", 0);
    
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
        ++nbEnveloppes;
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
        JavaThread::get()->getJVM()->unknownError("unknown bytecode");
    }
  }
}
