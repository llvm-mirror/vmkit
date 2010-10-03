//===---- JavaJITOpcodes.cpp - Reads and compiles opcodes -----------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG 0
#define JNJVM_COMPILE 0
#define JNJVM_EXECUTE 0

#include <cstring>

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

#include "mvm/JIT.h"

#include "debug.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "Reader.h"

#include "j3/J3Intrinsics.h"

#if DEBUG > 0 && (JNJVM_COMPILE > 0 || JNJVM_EXECUTE > 0)
#include "j3/OpcodeNames.def"
#endif

using namespace j3;
using namespace llvm;

uint8 arrayType(JavaMethod* meth, unsigned int t) {
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
    fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " unknown array type %d in %s.%s!\n", t,
                    UTF8Buffer(meth->classDef->name).cString(),
                    UTF8Buffer(meth->name).cString());
    abort();
  }
}

static inline uint32 WREAD_U1(Reader& reader, bool init, uint32 &i, bool& wide) {
  if (wide) {
    wide = init;
    i += 2;
    return reader.readU2();
  } else {
    i += 1;
    return reader.readU1();
  }
}

static inline sint32 WREAD_S1(Reader& reader, bool init, uint32 &i, bool &wide) {
  if (wide) {
    wide = init; 
    i += 2;
    return reader.readS2();
  } else {
    i += 1;
    return reader.readS1();
  }
}

static inline uint32 WCALC(uint32 n, bool& wide) {
  if (wide) {
    wide = false;
    return n << 1;
  } else {
    return n;
  }
}

void JavaJIT::compileOpcodes(Reader& reader, uint32 codeLength) {
  bool wide = false;
  uint32 jsrIndex = 0;
  uint32 start = reader.cursor;
  mvm::ThreadAllocator allocator;
  for(uint32 i = 0; i < codeLength; ++i) {
    reader.cursor = start + i;
    uint8 bytecode = reader.readU1();
    
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "\t[at %5d] %-5d ", i,
                bytecode);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "compiling ");
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecode]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n");
    
    Opinfo* opinfo = &(opcodeInfos[i]);
    
    if (opinfo->newBlock) {
      if (currentBlock->getTerminator() == 0) {
        // Load the exception object if we have branched to a handler.
        if (opinfo->handler) {
          Instruction* I = opinfo->newBlock->begin();
          PHINode * node = dyn_cast<PHINode>(I);
          assert(node && "Handler marlformed");
          Value* obj = pop();
          node->addIncoming(obj, currentBlock);
        }
        branch(*opinfo, currentBlock);
      }
      
      currentBlock = opinfo->newBlock;
      
      stack.clear();
      if (opinfo->handler) {
        Instruction* I = opinfo->newBlock->begin();
        assert(isa<PHINode>(I) && "Handler marlformed");
        // If it's a handler, put the exception object in the stack.
        new StoreInst(I, objectStack[0], "", currentBlock);
        stack.push_back(upcalls->OfObject);
        currentStackIndex = 1;
      } else {
        stack = opinfo->stack;
        currentStackIndex = stack.size();
      }
    }

    currentExceptionBlock = opinfo->exceptionBlock;
    
    // Update the line number information.
    if (opinfo->lineNumber)
      currentLineNumber = opinfo->lineNumber;
   
    currentCtpIndex = -1;
    currentBytecodeIndex = i;
    currentBytecode = bytecode;

    // To prevent a gcj bug with useless goto
    if (currentBlock->getTerminator() != 0) { 
      currentBlock = createBasicBlock("gcj bug");
    }
    
#if JNJVM_EXECUTE > 1
    {
      Value* args[3] = {
        ConstantInt::get(Type::getInt32Ty(*llvmContext), (int64_t)bytecode),
        ConstantInt::get(Type::getInt32Ty(*llvmContext), (int64_t)i),
        TheCompiler->getMethodInClass(compilingMethod)
      };
    
    
      CallInst::Create(intrinsics->PrintExecutionFunction, args, args + 3, "",
                       currentBlock);
    }
#endif
    
    switch (bytecode) {
      
      case NOP : break;

      case ACONST_NULL : 
        push(intrinsics->JavaObjectNullConstant, false);
        break;

      case ICONST_M1 :
        push(intrinsics->constantMinusOne, false);
        break;

      case ICONST_0 :
        push(intrinsics->constantZero, false);
        break;

      case ICONST_1 :
        push(intrinsics->constantOne, false);
        break;

      case ICONST_2 :
        push(intrinsics->constantTwo, false);
        break;

      case ICONST_3 :
        push(intrinsics->constantThree, false);
        break;

      case ICONST_4 :
        push(intrinsics->constantFour, false);
        break;

      case ICONST_5 :
        push(intrinsics->constantFive, false);
        break;

      case LCONST_0 :
        push(intrinsics->constantLongZero, false);
        push(intrinsics->constantZero, false);
        break;

      case LCONST_1 :
        push(intrinsics->constantLongOne, false);
        push(intrinsics->constantZero, false);
        break;

      case FCONST_0 :
        push(intrinsics->constantFloatZero, false);
        break;

      case FCONST_1 :
        push(intrinsics->constantFloatOne, false);
        break;
      
      case FCONST_2 :
        push(intrinsics->constantFloatTwo, false);
        break;
      
      case DCONST_0 :
        push(intrinsics->constantDoubleZero, false);
        push(intrinsics->constantZero, false);
        break;
      
      case DCONST_1 :
        push(intrinsics->constantDoubleOne, false);
        push(intrinsics->constantZero, false);
        break;

      case BIPUSH : 
        push(ConstantExpr::getSExt(ConstantInt::get(Type::getInt8Ty(*llvmContext),
                                                    reader.readU1()),
                                   Type::getInt32Ty(*llvmContext)), false);
        i++;
        break;

      case SIPUSH :
        push(ConstantExpr::getSExt(ConstantInt::get(Type::getInt16Ty(*llvmContext),
                                                    reader.readS2()),
                                   Type::getInt32Ty(*llvmContext)), false);
        i += 2;
        break;

      case LDC :
        loadConstant(reader.readU1());
        i++;
        break;

      case LDC_W :
        loadConstant(reader.readS2());
        i += 2;
        break;

      case LDC2_W :
        loadConstant(reader.readS2());
        i += 2;
        push(intrinsics->constantZero, false);
        break;

      case ILOAD :
        push(new LoadInst(intLocals[WREAD_U1(reader, false, i, wide)], "", false,
                          currentBlock), false);
        break;

      case LLOAD :
        push(new LoadInst(longLocals[WREAD_U1(reader, false, i, wide)], "", false,
                          currentBlock), false);
        push(intrinsics->constantZero, false);
        break;

      case FLOAD :
        push(new LoadInst(floatLocals[WREAD_U1(reader, false, i, wide)], "", false,
                          currentBlock), false);
        break;

      case DLOAD :
        push(new LoadInst(doubleLocals[WREAD_U1(reader, false, i, wide)], "", false,
                          currentBlock), false);
        push(intrinsics->constantZero, false);
        break;

      case ALOAD :
        push(new LoadInst(objectLocals[WREAD_U1(reader, false, i, wide)], "",
                          TheCompiler->useCooperativeGC(), currentBlock), false);
        break;
      
      case ILOAD_0 :
        push(new LoadInst(intLocals[0], "", false, currentBlock), false);
        break;
      
      case ILOAD_1 :
        push(new LoadInst(intLocals[1], "", false, currentBlock), false);
        break;

      case ILOAD_2 :
        push(new LoadInst(intLocals[2], "", false, currentBlock), false);
        break;

      case ILOAD_3 :
        push(new LoadInst(intLocals[3], "", false, currentBlock), false);
        break;
      
      case LLOAD_0 :
        push(new LoadInst(longLocals[0], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;

      case LLOAD_1 :
        push(new LoadInst(longLocals[1], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case LLOAD_2 :
        push(new LoadInst(longLocals[2], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case LLOAD_3 :
        push(new LoadInst(longLocals[3], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case FLOAD_0 :
        push(new LoadInst(floatLocals[0], "", false, currentBlock),
             false);
        break;
      
      case FLOAD_1 :
        push(new LoadInst(floatLocals[1], "", false, currentBlock),
             false);
        break;

      case FLOAD_2 :
        push(new LoadInst(floatLocals[2], "", false, currentBlock),
             false);
        break;

      case FLOAD_3 :
        push(new LoadInst(floatLocals[3], "", false, currentBlock),
             false);
        break;
      
      case DLOAD_0 :
        push(new LoadInst(doubleLocals[0], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;

      case DLOAD_1 :
        push(new LoadInst(doubleLocals[1], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case DLOAD_2 :
        push(new LoadInst(doubleLocals[2], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case DLOAD_3 :
        push(new LoadInst(doubleLocals[3], "", false, currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case ALOAD_0 :
        push(new LoadInst(objectLocals[0], "", TheCompiler->useCooperativeGC(), currentBlock),
             false);
        break;
      
      case ALOAD_1 :
        push(new LoadInst(objectLocals[1], "", TheCompiler->useCooperativeGC(), currentBlock),
             false);
        break;

      case ALOAD_2 :
        push(new LoadInst(objectLocals[2], "", TheCompiler->useCooperativeGC(), currentBlock),
             false);
        break;

      case ALOAD_3 :
        push(new LoadInst(objectLocals[3], "", TheCompiler->useCooperativeGC(), currentBlock),
             false);
        break;
      
      case IALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index, 
                                         intrinsics->JavaArraySInt32Type);
        push(new LoadInst(ptr, "", currentBlock), false);
        break;
      }

      case LALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayLongType);
        push(new LoadInst(ptr, "", currentBlock), false);
        push(intrinsics->constantZero, false);
        break;
      }

      case FALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayFloatType);
        push(new LoadInst(ptr, "", currentBlock), false);
        break;
      }

      case DALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayDoubleType);
        push(new LoadInst(ptr, "", currentBlock), false);
        push(intrinsics->constantZero, false);
        break;
      }

      case AALOAD : {
        Value* index = pop();
        CommonClass* cl = topTypeInfo();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayObjectType);
        
        if (cl->isArray()) cl = cl->asArrayClass()->baseClass();
        push(new LoadInst(ptr, "", currentBlock), false, cl);
        break;
      }

      case BALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArraySInt8Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case CALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayUInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new ZExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case SALOAD : {
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArraySInt16Type);
        Value* val = new LoadInst(ptr, "", currentBlock);
        push(new SExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case ISTORE : {
        Value* val = popAsInt();
        new StoreInst(val, intLocals[WREAD_U1(reader, false, i, wide)],
                      false, currentBlock);
        break;
      }
      
      case LSTORE :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), longLocals[WREAD_U1(reader, false, i, wide)],
                      false, currentBlock);
        break;
      
      case FSTORE :
        new StoreInst(pop(), floatLocals[WREAD_U1(reader, false, i, wide)],
                      false, currentBlock);
        break;
      
      case DSTORE :
        pop(); // remove the 0 on the stack
        new StoreInst(pop(), doubleLocals[WREAD_U1(reader, false, i, wide)],
                      false, currentBlock);
        break;

      case ASTORE : {
        CommonClass* cl = topTypeInfo();
        Instruction* V =
          new StoreInst(pop(), objectLocals[WREAD_U1(reader, false, i, wide)],
                        false, currentBlock);
        addHighLevelType(V, cl);
        break;
      } 
      case ISTORE_0 : {
        Value* val = pop();
        if (val->getType() != Type::getInt32Ty(*llvmContext)) // int8 and int16
          val = new ZExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock);
        new StoreInst(val, intLocals[0], false, currentBlock);
        break;
      }
      
      case ISTORE_1 : {
        Value* val = pop();
        if (val->getType() != Type::getInt32Ty(*llvmContext)) // int8 and int16
          val = new ZExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock);
        new StoreInst(val, intLocals[1], false, currentBlock);
        break;
      }

      case ISTORE_2 : {
        Value* val = pop();
        if (val->getType() != Type::getInt32Ty(*llvmContext)) // int8 and int16
          val = new ZExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock);
        new StoreInst(val, intLocals[2], false, currentBlock);
        break;
      }

      case ISTORE_3 : {
        Value* val = pop();
        if (val->getType() != Type::getInt32Ty(*llvmContext)) // int8 and int16
          val = new ZExtInst(val, Type::getInt32Ty(*llvmContext), "", currentBlock);
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
      
      case ASTORE_0 : {
        CommonClass* cl = topTypeInfo();
        Instruction* V = new StoreInst(pop(), objectLocals[0], false,
                                       currentBlock);
        addHighLevelType(V, cl);
        break;
      }
      
      case ASTORE_1 : {
        CommonClass* cl = topTypeInfo();
        Instruction* V = new StoreInst(pop(), objectLocals[1], false,
                                       currentBlock);
        addHighLevelType(V, cl);
        break;
      }
      
      case ASTORE_2 : {
        CommonClass* cl = topTypeInfo();
        Instruction* V = new StoreInst(pop(), objectLocals[2], false,
                                       currentBlock);
        addHighLevelType(V, cl);
        break;
      }
      
      case ASTORE_3 : {
        CommonClass* cl = topTypeInfo();
        Instruction* V = new StoreInst(pop(), objectLocals[3], false,
                                       currentBlock);
        addHighLevelType(V, cl);
        break;
      }
      
      case IASTORE : {
        Value* val = popAsInt();
        Value* index = popAsInt();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArraySInt32Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case LASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayLongType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case FASTORE : {
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayFloatType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case DASTORE : {
        pop(); // remove the 0 on stack
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayDoubleType);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }
      
      case AASTORE : {
        if (TheCompiler->hasExceptionsEnabled()) {
          // Get val and object and don't pop them: IsAssignableFromFunction
          // may go into runtime and we don't want values in registers at that
          // point.
          Value* val = new LoadInst(objectStack[currentStackIndex - 1], "",
                                    TheCompiler->useCooperativeGC(),
                                    currentBlock);
          Value* obj = new LoadInst(objectStack[currentStackIndex - 3], "",
                                    TheCompiler->useCooperativeGC(),
                                    currentBlock);
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val,
                                    intrinsics->JavaObjectNullConstant, "");

          BasicBlock* endBlock = createBasicBlock("end array store check");
          BasicBlock* checkBlock = createBasicBlock("array store check");
          BasicBlock* exceptionBlock = 
            createBasicBlock("array store exception");
          BranchInst::Create(endBlock, checkBlock, cmp, currentBlock);
          currentBlock = checkBlock;
        
          Value* valVT = CallInst::Create(intrinsics->GetVTFunction, val, "",
                                          currentBlock);
         
          Value* objVT = CallInst::Create(intrinsics->GetVTFunction, obj, "",
                                          currentBlock);
          objVT = CallInst::Create(intrinsics->GetBaseClassVTFromVTFunction, objVT,
                                   "", currentBlock);
          
          Value* VTArgs[2] = { valVT, objVT };
          
          Value* res = CallInst::Create(intrinsics->IsAssignableFromFunction,
                                        VTArgs, VTArgs + 2, "", currentBlock);

          BranchInst::Create(endBlock, exceptionBlock, res, currentBlock);
          
          currentBlock = exceptionBlock;
          throwException(intrinsics->ArrayStoreExceptionFunction, VTArgs, 2);

          currentBlock = endBlock;
        }
        Value* val = pop();
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayObjectType);

        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case BASTORE : {
        Value* val = pop();
        if (val->getType() != Type::getInt8Ty(*llvmContext)) {
          val = new TruncInst(val, Type::getInt8Ty(*llvmContext), "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArraySInt8Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case CASTORE : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::getInt32Ty(*llvmContext)) {
          val = new TruncInst(val, Type::getInt16Ty(*llvmContext), "", currentBlock);
        } else if (type == Type::getInt8Ty(*llvmContext)) {
          val = new ZExtInst(val, Type::getInt16Ty(*llvmContext), "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArrayUInt16Type);
        new StoreInst(val, ptr, false, currentBlock);
        break;
      }

      case SASTORE : {
        Value* val = pop();
        const Type* type = val->getType();
        if (type == Type::getInt32Ty(*llvmContext)) {
          val = new TruncInst(val, Type::getInt16Ty(*llvmContext), "", currentBlock);
        } else if (type == Type::getInt8Ty(*llvmContext)) {
          val = new SExtInst(val, Type::getInt16Ty(*llvmContext), "", currentBlock);
        }
        Value* index = pop();
        Value* obj = pop();
        Value* ptr = verifyAndComputePtr(obj, index,
                                         intrinsics->JavaArraySInt16Type);
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
        push(top(), false, topTypeInfo());
        break;

      case DUP_X1 : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        
        push(one, false, oneCl);
        push(two, false, twoCl);
        push(one, false, oneCl);
        break;
      }

      case DUP_X2 : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        CommonClass* threeCl = topTypeInfo();
        Value* three = pop();

        push(one, false, oneCl);
        push(three, false, threeCl);
        push(two, false, twoCl);
        push(one, false, oneCl);
        break;
      }

      case DUP2 : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        
        push(two, false, twoCl);
        push(one, false, oneCl);
        push(two, false, twoCl);
        push(one, false, oneCl);
        break;
      }

      case DUP2_X1 : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        CommonClass* threeCl = topTypeInfo();
        Value* three = pop();

        push(two, false, twoCl);
        push(one, false, oneCl);

        push(three, false, threeCl);
        push(two, false, twoCl);
        push(one, false, oneCl);

        break;
      }

      case DUP2_X2 : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        CommonClass* threeCl = topTypeInfo();
        Value* three = pop();
        CommonClass* fourCl = topTypeInfo();
        Value* four = pop();

        push(two, false, twoCl);
        push(one, false, oneCl);
        
        push(four, false, fourCl);
        push(three, false, threeCl);
        push(two, false, twoCl);
        push(one, false, oneCl);

        break;
      }

      case SWAP : {
        CommonClass* oneCl = topTypeInfo();
        Value* one = pop();
        CommonClass* twoCl = topTypeInfo();
        Value* two = pop();
        push(one, false, oneCl);
        push(two, false, twoCl);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case FADD : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateFAdd(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DADD : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateFAdd(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case FSUB : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateFSub(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DSUB : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateFSub(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case FMUL : {
        Value* val2 = pop();
        Value* val1 = pop();
        push(BinaryOperator::CreateFMul(val1, val2, "", currentBlock),
             false);
        break;
      }

      case DMUL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        push(BinaryOperator::CreateFMul(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      }

      case IDIV : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        if (TheCompiler->hasExceptionsEnabled()) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                    intrinsics->constantZero, "");
          BasicBlock* ifFalse = createBasicBlock("non null div");
          BasicBlock* ifTrue = createBasicBlock("null div");

          BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
          currentBlock = ifTrue;
          throwException(intrinsics->ArithmeticExceptionFunction, 0, 0);
          currentBlock = ifFalse;  
        }
        Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                  intrinsics->constantMinusOne, "");
        BasicBlock* ifFalse = createBasicBlock("non -1 div");
        BasicBlock* ifTrue = createBasicBlock("-1 div");
        BasicBlock* endBlock = createBasicBlock("End division");
        PHINode* node = PHINode::Create(val1->getType(), "", endBlock);
        BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
        currentBlock = ifTrue;
        node->addIncoming(BinaryOperator::CreateSub(intrinsics->constantZero,
                                                    val1, "", currentBlock),
                          currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        currentBlock = ifFalse;
        node->addIncoming(
            BinaryOperator::CreateSDiv(val1, val2, "", currentBlock),
            currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        currentBlock = endBlock;
        push(node, false);
        break;
      }

      case LDIV : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        if (TheCompiler->hasExceptionsEnabled()) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                    intrinsics->constantLongZero, "");
          BasicBlock* ifFalse = createBasicBlock("non null div");
          BasicBlock* ifTrue = createBasicBlock("null div");

          BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
          currentBlock = ifTrue;
          throwException(intrinsics->ArithmeticExceptionFunction, 0, 0);
          currentBlock = ifFalse;
        }
        push(BinaryOperator::CreateSDiv(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case IREM : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        if (TheCompiler->hasExceptionsEnabled()) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                    intrinsics->constantZero, "");
          BasicBlock* ifFalse = createBasicBlock("non null rem");
          BasicBlock* ifTrue = createBasicBlock("null rem");

          BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
          currentBlock = ifTrue;
          throwException(intrinsics->ArithmeticExceptionFunction, 0, 0);
          currentBlock = ifFalse;
        }
        Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                  intrinsics->constantMinusOne, "");
        BasicBlock* ifFalse = createBasicBlock("non -1 rem");
        BasicBlock* endBlock = createBasicBlock("end block");
        PHINode* node = PHINode::Create(val1->getType(), "", endBlock);
        node->addIncoming(intrinsics->constantZero, currentBlock);
        BranchInst::Create(endBlock, ifFalse, cmp, currentBlock);
        currentBlock = ifFalse;
        node->addIncoming(
            BinaryOperator::CreateSRem(val1, val2, "", currentBlock),
            currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        currentBlock = endBlock;
        push(node, false);
        break;
      }

      case LREM : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        if (TheCompiler->hasExceptionsEnabled()) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val2,
                                    intrinsics->constantLongZero, "");
          BasicBlock* ifFalse = createBasicBlock("non null div");
          BasicBlock* ifTrue = createBasicBlock("null div");

          BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
          currentBlock = ifTrue;
          throwException(intrinsics->ArithmeticExceptionFunction, 0, 0);
          currentBlock = ifFalse;
        }
        push(BinaryOperator::CreateSRem(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case INEG :
        push(BinaryOperator::CreateSub(
                              intrinsics->constantZero,
                              popAsInt(), "", currentBlock),
             false);
        break;
      
      case LNEG : {
        pop();
        push(BinaryOperator::CreateSub(
                              intrinsics->constantLongZero,
                              pop(), "", currentBlock), false);
        push(intrinsics->constantZero, false);
        break;
      }

      case FNEG :
        push(BinaryOperator::CreateFSub(
                              intrinsics->constantFloatMinusZero,
                              pop(), "", currentBlock), false);
        break;
      
      case DNEG : {
        pop();
        push(BinaryOperator::CreateFSub(
                              intrinsics->constantDoubleMinusZero,
                              pop(), "", currentBlock), false);
        push(intrinsics->constantZero, false);
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
        Value* val2 = new ZExtInst(pop(), Type::getInt64Ty(*llvmContext), "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateShl(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        Value* val2 = new ZExtInst(pop(), Type::getInt64Ty(*llvmContext), "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateAShr(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      }

      case IUSHR : {
        Value* val2 = popAsInt();
        Value* val1 = popAsInt();
        Value* mask = ConstantInt::get(Type::getInt32Ty(*llvmContext), 0x1F);
        val2 = BinaryOperator::CreateAnd(val2, mask, "", currentBlock);
        push(BinaryOperator::CreateLShr(val1, val2, "", currentBlock),
             false);
        break;
      }

      case LUSHR : {
        Value* val2 = new ZExtInst(pop(), Type::getInt64Ty(*llvmContext), "", currentBlock);
        Value* mask = ConstantInt::get(Type::getInt64Ty(*llvmContext), 0x3F);
        val2 = BinaryOperator::CreateAnd(val2, mask, "", currentBlock);
        pop(); // remove the 0 on the stack
        Value* val1 = pop();
        push(BinaryOperator::CreateLShr(val1, val2, "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
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
        push(intrinsics->constantZero, false);
        break;
      }

      case IINC : {
        uint16 idx = WREAD_U1(reader, true, i, wide);
        sint16 val = WREAD_S1(reader, false, i, wide);
        llvm::Value* add = BinaryOperator::CreateAdd(
            new LoadInst(intLocals[idx], "", currentBlock), 
            ConstantInt::get(Type::getInt32Ty(*llvmContext), val), "",
            currentBlock);
        new StoreInst(add, intLocals[idx], false, currentBlock);
        break;
      }

      case I2L :
        push(new SExtInst(pop(), llvm::Type::getInt64Ty(*llvmContext), "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;

      case I2F :
        push(new SIToFPInst(pop(), llvm::Type::getFloatTy(*llvmContext), "", currentBlock),
             false);
        break;
        
      case I2D :
        push(new SIToFPInst(pop(), llvm::Type::getDoubleTy(*llvmContext), "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case L2I :
        pop();
        push(new TruncInst(pop(), llvm::Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      
      case L2F :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::getFloatTy(*llvmContext), "", currentBlock),
             false);
        break;
      
      case L2D :
        pop();
        push(new SIToFPInst(pop(), llvm::Type::getDoubleTy(*llvmContext), "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case F2I : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(*currentBlock, FCmpInst::FCMP_ONE,
                                         val, val, "");
        
        BasicBlock* res = createBasicBlock("F2I");
        PHINode* node = PHINode::Create(llvm::Type::getInt32Ty(*llvmContext), "", res);
        node->addIncoming(intrinsics->constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OGE, val, 
                            intrinsics->constantMaxIntFloat, "");

        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMaxInt,
                          currentBlock);

        currentBlock = cont;

        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OLE, val,
                            intrinsics->constantMinIntFloat, "");
        
        cont = createBasicBlock("F2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::getInt32Ty(*llvmContext), "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;

        push(node, false);
        break;
      }

      case F2L : {
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(*currentBlock, FCmpInst::FCMP_ONE,
                                         val, val, "");
        
        BasicBlock* res = createBasicBlock("F2L");
        PHINode* node = PHINode::Create(llvm::Type::getInt64Ty(*llvmContext), "", res);
        node->addIncoming(intrinsics->constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OGE, val, 
                            intrinsics->constantMaxLongFloat, "");

        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMaxLong, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OLE, val, 
                            intrinsics->constantMinLongFloat, "");
        
        cont = createBasicBlock("F2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::getInt64Ty(*llvmContext), "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, false);
        push(intrinsics->constantZero, false);
        break;
      }

      case F2D :
        push(new FPExtInst(pop(), llvm::Type::getDoubleTy(*llvmContext), "", currentBlock),
             false);
        push(intrinsics->constantZero, false);
        break;
      
      case D2I : {
        pop(); // remove the 0 on the stack
        llvm::Value* val = pop();
        llvm::Value* test = new FCmpInst(*currentBlock, FCmpInst::FCMP_ONE,
                                         val, val, "");
        
        BasicBlock* res = createBasicBlock("D2I");
        PHINode* node = PHINode::Create(llvm::Type::getInt32Ty(*llvmContext), "", res);
        node->addIncoming(intrinsics->constantZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OGE, val,
                            intrinsics->constantMaxIntDouble, "");

        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMaxInt, currentBlock);

        currentBlock = cont;

        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OLE, val,
                            intrinsics->constantMinIntDouble, "");
        
        cont = createBasicBlock("D2I");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMinInt, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::getInt32Ty(*llvmContext), "",
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
        llvm::Value* test = new FCmpInst(*currentBlock, FCmpInst::FCMP_ONE,
                                         val, val, "");
        
        BasicBlock* res = createBasicBlock("D2L");
        PHINode* node = PHINode::Create(llvm::Type::getInt64Ty(*llvmContext), "", res);
        node->addIncoming(intrinsics->constantLongZero, currentBlock);
        BasicBlock* cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);

        currentBlock = cont;
        
        test = new FCmpInst(*currentBlock, FCmpInst::FCMP_OGE, val,
                            intrinsics->constantMaxLongDouble, "");

        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMaxLong, currentBlock);

        currentBlock = cont;

        test = 
          new FCmpInst(*currentBlock, FCmpInst::FCMP_OLE, val,
                       intrinsics->constantMinLongDouble, "");
        
        cont = createBasicBlock("D2L");

        BranchInst::Create(res, cont, test, currentBlock);
        node->addIncoming(intrinsics->constantMinLong, currentBlock);
        
        currentBlock = cont;
        llvm::Value* newVal = new FPToSIInst(val, Type::getInt64Ty(*llvmContext), "",
                                             currentBlock);
        BranchInst::Create(res, currentBlock);

        node->addIncoming(newVal, currentBlock);

        currentBlock = res;
        
        push(node, false);
        push(intrinsics->constantZero, false);
        break;
      }

      case D2F :
        pop(); // remove the 0 on the stack
        push(new FPTruncInst(pop(), llvm::Type::getFloatTy(*llvmContext), "", currentBlock),
             false);
        break;

      case I2B : {
        Value* val = pop();
        if (val->getType() == Type::getInt32Ty(*llvmContext)) {
          val = new TruncInst(val, llvm::Type::getInt8Ty(*llvmContext), "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case I2C : {
        Value* val = pop();
        if (val->getType() == Type::getInt32Ty(*llvmContext)) {
          val = new TruncInst(val, llvm::Type::getInt16Ty(*llvmContext), "", currentBlock);
        }
        push(new ZExtInst(val, llvm::Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case I2S : {
        Value* val = pop();
        if (val->getType() == Type::getInt32Ty(*llvmContext)) {
          val = new TruncInst(val, llvm::Type::getInt16Ty(*llvmContext), "", currentBlock);
        }
        push(new SExtInst(val, llvm::Type::getInt32Ty(*llvmContext), "", currentBlock),
             false);
        break;
      }

      case LCMP : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();

        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val1,
                                         val2, "");
        
        BasicBlock* cont = createBasicBlock("LCMP");
        BasicBlock* res = createBasicBlock("LCMP");
        PHINode* node = PHINode::Create(llvm::Type::getInt32Ty(*llvmContext), "", res);
        node->addIncoming(intrinsics->constantZero, currentBlock);
        
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;

        test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLT, val1, val2, "");
        node->addIncoming(intrinsics->constantMinusOne, currentBlock);

        cont = createBasicBlock("LCMP");
        BranchInst::Create(res, cont, test, currentBlock);
        currentBlock = cont;
        node->addIncoming(intrinsics->constantOne, currentBlock);
        BranchInst::Create(res, currentBlock);
        currentBlock = res;
        
        push(node, false);
        break;
      }

      case FCMPL : {
        llvm::Value* val2 = pop();
        llvm::Value* val1 = pop();
        compareFP(val1, val2, Type::getFloatTy(*llvmContext), false);
        break;
      }

      case FCMPG : {
        llvm::Value* val2 = pop();
        llvm::Value* val1 = pop();
        compareFP(val1, val2, Type::getFloatTy(*llvmContext), true);
        break;
      }

      case DCMPL : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        
        compareFP(val1, val2, Type::getDoubleTy(*llvmContext), false);
        break;
      }

      case DCMPG : {
        pop();
        llvm::Value* val2 = pop();
        pop();
        llvm::Value* val1 = pop();
        
        compareFP(val1, val2, Type::getDoubleTy(*llvmContext), false);
        break;
      }

      case IFEQ : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;

        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFEQ");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IFNE : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFNE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IFLT : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLT, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFLT");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IFGE : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGE, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFGE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IFGT : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFGT");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IFLE : {
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        Value* op = pop();
        const Type* type = op->getType();
        Constant* val = Constant::getNullValue(type);
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLE, op,
                                         val, "");
        BasicBlock* ifFalse = createBasicBlock("false IFLE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPEQ : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val1,
                                         val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPEQ");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPNE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, val1,
                                         val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPNE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPLT : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLT,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_IFCMPLT");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }
        
      case IF_ICMPGE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGE,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPGE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ICMPGT : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPGT");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }
      
      case IF_ICMPLE : {
        Value *val2 = popAsInt();
        Value *val1 = popAsInt();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLE,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ICMPLE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ACMPEQ : {
        Value *val2 = pop();
        Value *val1 = pop();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ACMPEQ");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case IF_ACMPNE : {
        Value *val2 = pop();
        Value *val1 = pop();
        uint32 tmp = i;
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE,
                                         val1, val2, "");
        BasicBlock* ifFalse = createBasicBlock("false IF_ACMPNE");
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }

      case GOTO : {
        uint32 tmp = i;
        branch(opcodeInfos[tmp + reader.readS2()],
               currentBlock);
        i += 2;
        break;
      }
      
      case JSR : {
        uint32 tmp = i;
        uint32 index = jsrIndex | 1;
        jsrIndex += 2;
        Value* expr = ConstantExpr::getIntToPtr(
                                    ConstantInt::get(Type::getInt64Ty(*llvmContext),
                                                     uint64_t (index)),
                                    intrinsics->JavaObjectType);
        push(expr, false);
        branch(opcodeInfos[tmp + reader.readS2()], currentBlock);
        i += 2;
        break;
      }

      case RET : {
        uint8 local = reader.readU1();
        i += 1;
        Value* _val = new LoadInst(
            objectLocals[local], "", TheCompiler->useCooperativeGC(), currentBlock);
        Value* val = new PtrToIntInst(_val, Type::getInt32Ty(*llvmContext), "", currentBlock);
        SwitchInst* inst = SwitchInst::Create(val, jsrs[0], jsrs.size(),
                                          currentBlock);
        
        uint32 index = 0;
        for (std::vector<BasicBlock*>::iterator i = jsrs.begin(), 
            e = jsrs.end(); i!= e; ++i, index += 2) {
          inst->addCase(ConstantInt::get(Type::getInt32Ty(*llvmContext), index | 1), *i);
        }

        break;
      }

      case TABLESWITCH : {
        uint32 tmp = i;
        uint32 reste = (i + 1) & 3;
        uint32 filled = reste ?  (4 - reste) : 0;
        i += filled;
        reader.cursor += filled;
        Opinfo& def = opcodeInfos[tmp + reader.readU4()];
        i += 4;

        sint32 low = reader.readS4();
        i += 4;
        sint32 high = reader.readS4() + 1;
        i += 4;
        
        Value* index = pop(); 
        
        const llvm::Type* type = index->getType();
        for (sint32 cur = low; cur < high; ++cur) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ,
                                    ConstantInt::get(type, cur), index, "");
          BasicBlock* falseBlock = createBasicBlock("continue tableswitch");
          Opinfo& info = opcodeInfos[tmp + reader.readU4()];
          i += 4;
          branch(cmp, info.newBlock, falseBlock, currentBlock, info);
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
        reader.cursor += filled;
        Opinfo& def = opcodeInfos[tmp + reader.readU4()];
        i += 4;
        uint32 nbs = reader.readU4();
        i += 4;
        
        Value* key = pop();
        for (uint32 cur = 0; cur < nbs; ++cur) {
          Value* val = ConstantInt::get(Type::getInt32Ty(*llvmContext), reader.readU4());
          i += 4;
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val, key,
                                    "");
          BasicBlock* falseBlock = createBasicBlock("continue lookupswitch");
          Opinfo& info = opcodeInfos[tmp + reader.readU4()];
          i += 4;
          branch(cmp, info.newBlock, falseBlock, currentBlock, info);
          currentBlock = falseBlock;
        }
        branch(def, currentBlock);
        i = tmp + 8 + filled + (nbs << 3);
        break;
      }
      case IRETURN : {
        Value* val = pop();
        assert(val->getType()->isIntegerTy());
        convertValue(val, endNode->getType(), currentBlock, false);
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
      
      case RETURN : {
        // Prevent a javac bug.
        if (endNode != 0) {
          endNode->addIncoming(Constant::getNullValue(endNode->getType()),
                               currentBlock);
        }
        BranchInst::Create(endBlock, currentBlock);
        break;
      }

      case GETSTATIC : {
        uint16 index = reader.readU2();
        i += 2;
        getStaticField(index);
        break;
      }

      case PUTSTATIC : {
        uint16 index = reader.readU2();
        i += 2;
        setStaticField(index);
        break;
      }

      case GETFIELD : {
        uint16 index = reader.readU2();
        i += 2;
        getVirtualField(index);
        break;
      }

      case PUTFIELD : {
        uint16 index = reader.readU2();
        i += 2;
        setVirtualField(index);
        break;
      }

      case INVOKEVIRTUAL : {
        uint16 index = reader.readU2();
        i += 2;
        currentCtpIndex = index;
        invokeVirtual(index);
        break;
      }

      case INVOKESPECIAL : {
        uint16 index = reader.readU2();
        i += 2;
        currentCtpIndex = index;
        invokeSpecial(index);
        break;
      }

      case INVOKESTATIC : {
        uint16 index = reader.readU2();
        i += 2;
        currentCtpIndex = index;
        invokeStatic(index);
        break;
      }

      case INVOKEINTERFACE : {
        uint16 index = reader.readU2();
        i += 2;
        currentCtpIndex = index;
        invokeInterface(index);
        i += 2;
        break;
      }

      case NEW : {
        uint16 index = reader.readU2();
        i += 2;
        invokeNew(index);
        break;
      }

      case NEWARRAY :
      case ANEWARRAY : {
        
        Constant* sizeElement = 0;
        Value* TheVT = 0;
        Value* valCl = 0;
        UserClassArray* dcl = 0;

        if (bytecode == NEWARRAY) {
          uint8 id = reader.readU1();
          i += 1;
          uint8 charId = arrayType(compilingMethod, id);
#ifndef ISOLATE_SHARING
          JnjvmBootstrapLoader* loader = 
            compilingClass->classLoader->bootstrapLoader;
          dcl = loader->getArrayClass(id);
          valCl = TheCompiler->getNativeClass(dcl);
#else
          Value* args[2] = { isolateLocal,
                             ConstantInt::get(Type::getInt32Ty(*llvmContext), id - 4) };
          valCl = CallInst::Create(intrinsics->GetJnjvmArrayClassFunction,
                                   args, args + 2, "", currentBlock);
#endif

          LLVMAssessorInfo& LAI = TheCompiler->AssessorInfo[charId];
          sizeElement = ConstantInt::get(Type::getInt32Ty(*llvmContext),
                                                    LAI.logSizeInBytesConstant);
          if (TheCompiler->isStaticCompiling() &&
              valCl->getType() != intrinsics->JavaClassArrayType) {
            valCl = new LoadInst(valCl, "", currentBlock);
            TheVT = CallInst::Create(intrinsics->GetVTFromClassArrayFunction,
                                     valCl, "", currentBlock);
          } else {
            TheVT = TheCompiler->getVirtualTable(dcl->virtualVT);
          }
        } else {
          uint16 index = reader.readU2();
          i += 2;
          CommonClass* cl =
            compilingClass->ctpInfo->getMethodClassIfLoaded(index); 

          if (cl && (!cl->isClass() || cl->asClass()->isResolved())) {
            JnjvmClassLoader* JCL = cl->classLoader;
            const UTF8* arrayName = JCL->constructArrayName(1, cl->name);
          
            dcl = JCL->constructArray(arrayName);
            valCl = TheCompiler->getNativeClass(dcl);
            
            // If we're static compiling and the class is not a class we
            // are compiling, the result of getNativeClass is a pointer to
            // the class. Load it.
            if (TheCompiler->isStaticCompiling() && 
                valCl->getType() != intrinsics->JavaClassArrayType) {
              valCl = new LoadInst(valCl, "", currentBlock);
              TheVT = CallInst::Create(intrinsics->GetVTFromClassArrayFunction,
                                       valCl, "", currentBlock);
            } else {
              TheVT = TheCompiler->getVirtualTable(dcl->virtualVT);
            }

          } else {
            const llvm::Type* Ty = 
              PointerType::getUnqual(intrinsics->VTType);
            Value* args[3]= { TheCompiler->getNativeClass(compilingClass),
                              ConstantInt::get(Type::getInt32Ty(*llvmContext), index),
                              Constant::getNullValue(Ty) };
            TheVT = CallInst::Create(intrinsics->GetArrayClassFunction, args,
                                     args + 3, "", currentBlock);
          }

          sizeElement = intrinsics->constantPtrLogSize;
        }
        Value* arg1 = popAsInt();

        if (TheCompiler->hasExceptionsEnabled()) {
          Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_SLT, arg1,
                                    intrinsics->constantZero, "");

          BasicBlock* BB1 = createBasicBlock("");
          BasicBlock* BB2 = createBasicBlock("");

          BranchInst::Create(BB1, BB2, cmp, currentBlock);
          currentBlock = BB1;
          throwException(intrinsics->NegativeArraySizeExceptionFunction, arg1);
          currentBlock = BB2;
        
          cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT, arg1,
                             intrinsics->MaxArraySizeConstant, "");

          BB1 = createBasicBlock("");
          BB2 = createBasicBlock("");

          BranchInst::Create(BB1, BB2, cmp, currentBlock);
          currentBlock = BB1;
          throwException(intrinsics->OutOfMemoryErrorFunction, arg1);
          currentBlock = BB2;
        }
        
        Value* mult = BinaryOperator::CreateShl(arg1, sizeElement, "",
                                                currentBlock);
        Value* size =
          BinaryOperator::CreateAdd(intrinsics->JavaArraySizeConstant, mult,
                                    "", currentBlock);
        TheVT = new BitCastInst(TheVT, intrinsics->ptrType, "", currentBlock);
        Instruction* res = invoke(intrinsics->AllocateFunction, size, TheVT, "",
                                  currentBlock);
        Value* cast = new BitCastInst(res, intrinsics->JavaArrayType, "",
                                      currentBlock);

        // Set the size
        Value* gep4[2] = { intrinsics->constantZero,
                           intrinsics->JavaArraySizeOffsetConstant };
        Value* GEP = GetElementPtrInst::Create(cast, gep4, gep4 + 2,
                                               "", currentBlock);
        
        arg1 = new IntToPtrInst(arg1, intrinsics->ptrType, "", currentBlock);
        new StoreInst(arg1, GEP, currentBlock);
       
        addHighLevelType(res, dcl ? dcl : upcalls->ArrayOfObject);
        res = new BitCastInst(res, intrinsics->JavaObjectType, "", currentBlock);
        push(res, false, dcl ? dcl : upcalls->ArrayOfObject);

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
        throwException(arg);
        break;
      }

      case CHECKCAST :
        if (!TheCompiler->hasExceptionsEnabled()) {
          i += 2;
          reader.cursor += 2;
          break;
        }

      case INSTANCEOF : {
        
        bool checkcast = (bytecode == CHECKCAST);
        
        BasicBlock* exceptionCheckcast = 0;
        BasicBlock* endCheckcast = 0;

        uint16 index = reader.readU2();
        i += 2;
        UserCommonClass* cl = 0;
        Value* clVar = getResolvedCommonClass(index, true, &cl);
        Value* obj = top();
        Value* args[2] = { obj, clVar };
        Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, obj,
                                  intrinsics->JavaObjectNullConstant, "");
        BasicBlock* endBlock = createBasicBlock("end type compare");
        PHINode* node = PHINode::Create(Type::getInt1Ty(*llvmContext), "", endBlock);
        
        if (checkcast) {
          exceptionCheckcast = createBasicBlock("false checkcast");

        
          endCheckcast = createBasicBlock("null checkcast");
          BasicBlock* ifFalse = createBasicBlock("non null checkcast");

          BranchInst::Create(endCheckcast, ifFalse, cmp, currentBlock);
          currentBlock = exceptionCheckcast;
          throwException(intrinsics->ClassCastExceptionFunction, args, 2);
          currentBlock = ifFalse;
        } else {
          BasicBlock* ifFalse = createBasicBlock("false type compare");
          BranchInst::Create(endBlock, ifFalse, cmp, currentBlock);
          node->addIncoming(ConstantInt::getFalse(*llvmContext), currentBlock);
          currentBlock = ifFalse;
        }

        Value* TheVT = 0;
        if (!cl || TheCompiler->isStaticCompiling()) {
          TheVT = CallInst::Create(intrinsics->GetVTFromCommonClassFunction,
                                   clVar, "", currentBlock);
        } else {
          TheVT = TheCompiler->getVirtualTable(cl->virtualVT);
        }

        
        Value* objVT = CallInst::Create(intrinsics->GetVTFunction, obj, "",
                                       currentBlock);
        Value* classArgs[2] = { objVT, TheVT };
         
        Value* res = 0;
        if (cl) {
          if (cl->isSecondaryClass()) {
            res = CallInst::Create(intrinsics->IsSecondaryClassFunction,
                                   classArgs, classArgs + 2, "",
                                   currentBlock);
          } else {
            Value* inDisplay = CallInst::Create(intrinsics->GetDisplayFunction,
                                                objVT, "", currentBlock);
            
            uint32 depth = cl->virtualVT->depth;
            ConstantInt* CI = ConstantInt::get(Type::getInt32Ty(*llvmContext), depth);
            Value* displayArgs[2] = { inDisplay, CI };
            Value* VTInDisplay = 
              CallInst::Create(intrinsics->GetVTInDisplayFunction,
                               displayArgs, displayArgs + 2, "",
                               currentBlock);
             
            res = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, VTInDisplay,
                               TheVT, "");
          }
        } else {
          res = CallInst::Create(intrinsics->IsAssignableFromFunction,
                                 classArgs, classArgs + 2, "",
                                 currentBlock);
        }

        node->addIncoming(res, currentBlock);
        BranchInst::Create(endBlock, currentBlock);
        currentBlock = endBlock;

        if (checkcast) {
          BranchInst::Create(endCheckcast, exceptionCheckcast, node,
                             currentBlock);
          currentBlock = endCheckcast;
        } else {
          pop();
          push(new ZExtInst(node, Type::getInt32Ty(*llvmContext), "", currentBlock),
               false);
        }

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
        uint16 index = reader.readU2();
        i += 2;
        uint8 dim = reader.readU1();
        i += 1;
        
        UserCommonClass* dcl = 0; 
        Value* valCl = getResolvedCommonClass(index, true, &dcl);
        Value** args = (Value**)allocator.Allocate(sizeof(Value*) * (dim + 2));
        args[0] = valCl;
        args[1] = ConstantInt::get(Type::getInt32Ty(*llvmContext), dim);

        for (int cur = dim + 1; cur >= 2; --cur)
          args[cur] = pop();
        
        std::vector<Value*> Args;
        for (sint32 v = 0; v < dim + 2; ++v) {
          Args.push_back(args[v]);
        }
        push(invoke(intrinsics->MultiCallNewFunction, Args, "", currentBlock),
             false, dcl ? dcl : upcalls->ArrayOfObject);
        break;
      }

      case WIDE :
        wide = true;
        break;

      case IFNULL : {
        uint32 tmp = i;
        llvm::Value* val = pop();
        Constant* nil = Constant::getNullValue(val->getType());
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, val,
                                         nil, "");
        BasicBlock* ifFalse = createBasicBlock("true IFNULL");
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }
      
      case IFNONNULL : {
        uint32 tmp = i;
        llvm::Value* val = pop();
        Constant* nil = Constant::getNullValue(val->getType());
        llvm::Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, val,
                                         nil, "");
        BasicBlock* ifFalse = createBasicBlock("false IFNONNULL");
        Opinfo& ifTrueInfo = opcodeInfos[tmp + reader.readS2()];
        i += 2;
        BasicBlock* ifTrue = ifTrueInfo.newBlock;
        branch(test, ifTrue, ifFalse, currentBlock, ifTrueInfo);
        currentBlock = ifFalse;
        break;
      }
      
      default : {
        fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " unknown bytecode %d in %s.%s\n!", bytecode,
                    UTF8Buffer(compilingClass->name).cString(),
                    UTF8Buffer(compilingMethod->name).cString());
        abort();
      }
    } 
  }
}

void JavaJIT::exploreOpcodes(Reader& reader, uint32 codeLength) {
  bool wide = false;
  uint32 start = reader.cursor;
  for(uint32 i = 0; i < codeLength; ++i) {
    reader.cursor = start + i;
    uint8 bytecode = reader.readU1();
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "\t[at %5d] %-5d ", i,
                bytecode);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "exploring ");
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_CYAN, OpcodeNames[bytecode]);
    PRINT_DEBUG(JNJVM_COMPILE, 1, LIGHT_BLUE, "\n");
    
    switch (bytecode) {
     
      case NOP :
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
        i += WCALC(1, wide);
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
        i += WCALC(1, wide);
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
        i += WCALC(2, wide);
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
        uint16 index = tmp + reader.readU2();
        i += 2;
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("GOTO or IF*");
        break;
      }
      
      case JSR : {
        uint32 tmp = i;
        uint16 index = tmp + reader.readU2();
        i += 2;
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
        break;
      }

      case RET : ++i; break;

      case TABLESWITCH : {
        uint32 tmp = i;
        uint32 reste = (i + 1) & 3;
        uint32 filled = reste ? (4 - reste) : 0; 
        i += filled;
        reader.cursor += filled;
        uint32 index = tmp + reader.readU4();
        i += 4;
        if (!(opcodeInfos[index].newBlock)) {
          BasicBlock* block = createBasicBlock("tableswitch");
          opcodeInfos[index].newBlock = block;
        }
        uint32 low = reader.readU4();
        i += 4;
        uint32 high = reader.readU4() + 1;
        i += 4;
        uint32 depl = high - low;
        for (uint32 cur = 0; cur < depl; ++cur) {
          uint32 index2 = tmp + reader.readU4();
          i += 4;
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
        reader.cursor += filled;
        uint32 index = tmp + reader.readU4();
        i += 4;
        if (!(opcodeInfos[index].newBlock)) {
          BasicBlock* block = createBasicBlock("tableswitch");
          opcodeInfos[index].newBlock = block;
        }
        uint32 nbs = reader.readU4();
        i += 4;
        for (uint32 cur = 0; cur < nbs; ++cur) {
          i += 4;
          reader.cursor += 4;
          uint32 index2 = tmp + reader.readU4();
          i += 4;
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
        uint16 index = tmp + reader.readU2();
        i += 2;
        if (!(opcodeInfos[index].newBlock))
          opcodeInfos[index].newBlock = createBasicBlock("true IF*NULL");
        break;
      }


      default : {
        fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " unknown bytecode %d in %s.%s!\n", bytecode,
                    UTF8Buffer(compilingClass->name).cString(),
                    UTF8Buffer(compilingMethod->name).cString());
        abort();
      }
    }
  }
}
