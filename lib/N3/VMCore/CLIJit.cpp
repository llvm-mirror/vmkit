//===------------ CLIJit.cpp - CLI just in time compiler ------------------===//
//
//                              N3
//
// This file is distributed under the University Of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//#define DEBUG 0
//#define N3_COMPILE 0
//#define N3_EXECUTE 0

#include "debug.h"
#include "types.h"

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/CFG.h>
#include <llvm/Support/MutexGuard.h>



#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "Assembly.h"
#include "CLIAccess.h"
#include "CLIJit.h"
#include "CLIString.h"
#include "NativeUtil.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "Reader.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMCache.h"
#include "VMClass.h"
#include "VMThread.h"

using namespace llvm;
using namespace n3;

#include <iostream>

void Exception::print(mvm::PrintBuffer* buf) const {
  buf->write("Exception<>");
}

#ifdef WITH_TRACER
// for structs
static void traceStruct(VMCommonClass* cl, BasicBlock* block, Value* arg) {
#ifdef MULTIPLE_GC
  Value* GC = ++(block->getParent()->arg_begin());
#endif

  for (std::vector<VMField*>::iterator i = cl->virtualFields.begin(), 
            e = cl->virtualFields.end(); i!= e; ++i) {

    VMField* field = *i;
    if (field->signature->super == N3::pValue) {
      if (!field->signature->isPrimitive) {
        Value* ptr = GetElementPtrInst::Create(arg, field->offset, "",
                                           block);
        traceStruct(field->signature, block, ptr);
      } else if (field->signature == N3::pIntPtr || 
                 field->signature == N3::pUIntPtr)  {
        Value* valCast = new BitCastInst(arg, VMObject::llvmType, "", block);
#ifdef MULTIPLE_GC
        std::vector<Value*> Args;
        Args.push_back(valCast);
        Args.push_back(GC);
        CallInst::Create(CLIJit::markAndTraceLLVM, Args.begin(), Args.end(),
                         "", block);
#else
        CallInst::Create(CLIJit::markAndTraceLLVM, valCast, "", block);
#endif
      }
    } else if (field->signature->super != N3::pEnum) {
      Value* valCast = new BitCastInst(arg, VMObject::llvmType, "", block);
#ifdef MULTIPLE_GC
      std::vector<Value*> Args;
      Args.push_back(valCast);
      Args.push_back(GC);
      CallInst::Create(CLIJit::markAndTraceLLVM, Args.begin(), Args.end(),
                       "", block);
#else
      CallInst::Create(CLIJit::markAndTraceLLVM, valCast, "", block);
#endif
    }
  }
}


// Always classes
static void traceClass(VMCommonClass* cl, BasicBlock* block, Value* arg, 
                       std::vector<VMField*>& fields, bool boxed = false) {
#ifdef MULTIPLE_GC
  Value* GC = ++(block->getParent()->arg_begin());
#endif
  
  Constant* zero = mvm::jit::constantZero;
  for (std::vector<VMField*>::iterator i = fields.begin(), 
            e = fields.end(); i!= e; ++i) {
    VMField* field = *i;
    if (field->signature->super == N3::pValue) {
      std::vector<Value*> args; //size = 2
      args.push_back(zero);
      if (boxed) {
        args.push_back(ConstantInt::get(field->offset->getValue() + 1));
      } else {
        args.push_back(field->offset);
      }
      Value* ptr = GetElementPtrInst::Create(arg, args.begin(), args.end(), "",
                                         block);
      traceStruct(field->signature, block, ptr);
    } else if (field->signature->super != N3::pEnum) {
      std::vector<Value*> args; //size = 2
      args.push_back(zero);
      if (boxed) {
        args.push_back(ConstantInt::get(field->offset->getValue() + 1));
      } else {
        args.push_back(field->offset);
      }
      Value* ptr = GetElementPtrInst::Create(arg, args.begin(), args.end(), "",
                                         block);
      Value* val = new LoadInst(ptr, "", block);
      Value* valCast = new BitCastInst(val, VMObject::llvmType, "", block);
#ifdef MULTIPLE_GC
        std::vector<Value*> Args;
        Args.push_back(valCast);
        Args.push_back(GC);
        CallInst::Create(CLIJit::markAndTraceLLVM, Args.begin(), Args.end(),
                         "", block);
#else
      CallInst::Create(CLIJit::markAndTraceLLVM, valCast, "", block);
#endif
    }
  }
}
#endif

VirtualTable* CLIJit::makeArrayVT(VMClassArray* cl) {
  VirtualTable * res = malloc(VT_SIZE);
  memcpy(res, VMObject::VT, VT_SIZE);
#ifdef WITH_TRACER  
  Function* func = Function::Create(markAndTraceLLVMType,
                                GlobalValue::ExternalLinkage,
                                "markAndTraceObject",
                                cl->vm->module);
  Argument* arg = func->arg_begin();
#ifdef MULTIPLE_GC 
  Argument* GC = ++(func->arg_begin());
#endif
    // Constant Definitions
  Constant* const_int32_8 = mvm::jit::constantZero;
  ConstantInt* const_int32_9 = mvm::jit::constantOne;
  ConstantInt* const_int32_10 = mvm::jit::constantTwo;
  
  
  // Function Definitions
  
  {
    BasicBlock* label_entry = BasicBlock::Create("entry",func,0);
    BasicBlock* label_bb = BasicBlock::Create("bb",func,0);
    BasicBlock* label_return = BasicBlock::Create("return",func,0);
    
    Value* ptr_v = new BitCastInst(arg, cl->naturalType, "", label_entry);
    
    // Block entry (label_entry)
    std::vector<Value*> ptr_tmp918_indices;
    ptr_tmp918_indices.push_back(const_int32_8);
    ptr_tmp918_indices.push_back(const_int32_9);
    Instruction* ptr_tmp918 = 
      GetElementPtrInst::Create(ptr_v, ptr_tmp918_indices.begin(), 
                                ptr_tmp918_indices.end(), "tmp918", 
                                label_entry);
    LoadInst* int32_tmp1019 = new LoadInst(ptr_tmp918, "tmp1019", false, 
                                           label_entry);

    ICmpInst* int1_tmp1221 = new ICmpInst(ICmpInst::ICMP_SGT, int32_tmp1019, 
                                          const_int32_8, "tmp1221",
                                          label_entry);

    BranchInst::Create(label_bb, label_return, int1_tmp1221, label_entry);
    
    // Block bb (label_bb)
    Argument* fwdref_12 = new Argument(IntegerType::get(32));
    PHINode* int32_i_015_0 = PHINode::Create(Type::Int32Ty, "i.015.0", 
                                             label_bb);
    int32_i_015_0->reserveOperandSpace(2);
    int32_i_015_0->addIncoming(fwdref_12, label_bb);
    int32_i_015_0->addIncoming(const_int32_8, label_entry);
    
    std::vector<Value*> ptr_tmp3_indices;
    ptr_tmp3_indices.push_back(const_int32_8);
    ptr_tmp3_indices.push_back(const_int32_10);
    ptr_tmp3_indices.push_back(int32_i_015_0);
    Instruction* ptr_tmp3 = 
      GetElementPtrInst::Create(ptr_v, ptr_tmp3_indices.begin(), 
                                ptr_tmp3_indices.end(), "tmp3", label_bb);

    if (cl->baseClass->super == N3::pValue) {
      traceStruct(cl->baseClass, label_bb, ptr_tmp3);
    } else if (cl->baseClass->super != N3::pEnum) {
      LoadInst* ptr_tmp4 = new LoadInst(ptr_tmp3, "tmp4", false, label_bb);
      Value* arg = new BitCastInst(ptr_tmp4, VMObject::llvmType, "", label_bb);
#ifdef MULTIPLE_GC
      std::vector<Value*> Args;
      Args.push_back(arg);
      Args.push_back(GC);
      CallInst::Create(markAndTraceLLVM, Args.begin(), Args.end(), "",
                       label_bb);
#else
      CallInst::Create(markAndTraceLLVM, arg, "", label_bb);
#endif
    }
    BinaryOperator* int32_tmp6 = 
      BinaryOperator::create(Instruction::Add, int32_i_015_0, const_int32_9,
                             "tmp6", label_bb);
    LoadInst* int32_tmp10 = new LoadInst(ptr_tmp918, "tmp10", false, label_bb);
    ICmpInst* int1_tmp12 = new ICmpInst(ICmpInst::ICMP_SGT, int32_tmp10, 
                                        int32_tmp6, "tmp12", label_bb);
    BranchInst::Create(label_bb, label_return, int1_tmp12, label_bb);
    
    // Block return (label_return)
    ReturnInst::Create(label_return);
    
    // Resolve Forward References
    fwdref_12->replaceAllUsesWith(int32_tmp6); delete fwdref_12;
    
  }
  
  void* tracer = mvm::jit::executionEngine->getPointerToGlobal(func);
  ((void**)res)[VT_TRACER_OFFSET] = tracer;
  cl->virtualTracer = func;
#endif

  return res;
}

VirtualTable* CLIJit::makeVT(VMClass* cl, bool stat) {
  VirtualTable * res = malloc(VT_SIZE);
  memcpy(res, VMObject::VT, VT_SIZE);
#ifdef WITH_TRACER  
  const Type* type = stat ? cl->staticType : cl->virtualType;
  std::vector<VMField*> &fields = stat ? cl->staticFields : cl->virtualFields;
  
  Function* func = Function::Create(markAndTraceLLVMType,
                                GlobalValue::ExternalLinkage,
                                "markAndTraceObject",
                                cl->vm->module);

  Argument* arg = func->arg_begin();
#ifdef MULTIPLE_GC 
  Argument* GC = ++(func->arg_begin());
#endif
  BasicBlock* block = BasicBlock::Create("", func);
  llvm::Value* realArg = new BitCastInst(arg, type, "", block);
 
#ifdef MULTIPLE_GC
  std::vector<Value*> Args;
  Args.push_back(arg);
  Args.push_back(GC);
  if (stat || cl->super == 0) {
    CallInst::Create(vmObjectTracerLLVM, Args.begin(), Args.end(), "", block);
  } else {
    CallInst::Create(((VMClass*)cl->super)->virtualTracer, Args.begin(),
                     Args.end(), "", block);
  }
#else
  if (stat || cl->super == 0) {
    CallInst::Create(vmObjectTracerLLVM, arg, "", block);
  } else {
    CallInst::Create(((VMClass*)cl->super)->virtualTracer, arg, "", block);
  }
#endif
  
  traceClass(cl, block, realArg, fields, (cl->super == N3::pValue && !stat));
  ReturnInst::Create(block);

  void* tracer = mvm::jit::executionEngine->getPointerToGlobal(func);
  ((void**)res)[VT_TRACER_OFFSET] = tracer;
  
  if (!stat) {
    cl->virtualTracer = func;
  } else {
    cl->staticTracer = func;
  }
#endif
  return res;
}

BasicBlock* CLIJit::createBasicBlock(const char* name) {
  return BasicBlock::Create(name, llvmFunction);
}

void CLIJit::setCurrentBlock(BasicBlock* newBlock) {

  std::vector<Value*> newStack;
  uint32 index = 0;
  for (BasicBlock::iterator i = newBlock->begin(), e = newBlock->end(); i != e;
       ++i, ++index) {
    if (!(isa<PHINode>(i))) {
      break;
    } else {
      newStack.push_back(i);
    }
  }
  
  stack = newStack;
  currentBlock = newBlock;
}

extern void convertValue(Value*& val, const Type* t1, BasicBlock* currentBlock);

static void testPHINodes(BasicBlock* dest, BasicBlock* insert, CLIJit* jit) {
  if(dest->empty()) {
    for (std::vector<Value*>::iterator i = jit->stack.begin(),
            e = jit->stack.end(); i!= e; ++i) {
      Value* cur = (*i);
      PHINode* node = PHINode::Create(cur->getType(), "", dest);
      node->addIncoming(cur, insert);
    }
  } else {
    std::vector<Value*>::iterator stackit = jit->stack.begin();
    for (BasicBlock::iterator i = dest->begin(), e = dest->end(); i != e;
         ++i) {
      if (!(isa<PHINode>(i))) {
        break;
      } else {
        Instruction* ins = i;
        Value* cur = (*stackit);
        convertValue(cur, ins->getType(), insert);
        ((PHINode*)ins)->addIncoming(cur, insert);
        ++stackit;
      }
    }
  }
}

void CLIJit::branch(llvm::BasicBlock* dest, llvm::BasicBlock* insert) {
  testPHINodes(dest, insert, this);
  BranchInst::Create(dest, insert);
}

void CLIJit::branch(llvm::Value* test, llvm::BasicBlock* ifTrue,
                    llvm::BasicBlock* ifFalse, llvm::BasicBlock* insert) {  
  testPHINodes(ifTrue, insert, this);
  testPHINodes(ifFalse, insert, this);
  BranchInst::Create(ifTrue, ifFalse, test, insert);
}

Value* CLIJit::pop() {
  assert(stack.size());
  Value* ret = top();
  stack.pop_back();
  return ret;
}

Value* CLIJit::top() {
  return stack.back();
}

void CLIJit::push(Value* val) {
  assert(val);
  stack.push_back(val);
}

Value* CLIJit::changeType(Value* val, const Type* type) {
  const Type* valType = val->getType();
  if (type->isInteger()) {
    if (valType == PointerType::getUnqual(type)) {
      // in cast it's a struct
      val = new LoadInst(val, "", currentBlock);
    }
    else if (type->getPrimitiveSizeInBits() < 
             valType->getPrimitiveSizeInBits()) {
      val = new TruncInst(val, type, "", currentBlock);
    } else {
      val = new SExtInst(val, type, "", currentBlock);
    }
  } else if (type == Type::FloatTy) {
    val = new FPTruncInst(val, type, "", currentBlock);
  } else if (type == Type::DoubleTy) {
    val = new FPExtInst(val, type, "", currentBlock);
  } else {
    val = new BitCastInst(val, type, "", currentBlock);
  }
  return val;
}

void CLIJit::makeArgs(const FunctionType* type, std::vector<Value*>& Args,
                      bool structReturn) {
  uint32 size = type->getNumParams();
  Value** args = (Value**)alloca(sizeof(Value*) * size);
  sint32 index = size - 1;
  FunctionType::param_iterator e = type->param_end();
  e--;
  if (structReturn) { e--; index--; size--; }
  for (; index >= 0; --e, --index) {
    const Type* argType = (*e);
    Value* val = pop();
    if (val->getType() != argType) {
      val = changeType(val, argType);
    }
    args[index] = val;
  }

  for (uint32 i = 0; i < size; ++i) {
    Args.push_back(args[i]);
  }
}

Instruction* CLIJit::lowerMathOps(VMMethod* meth, 
                                  std::vector<Value*>& args) {
  
  if (meth->name == N3::sqrt) {
    return CallInst::Create(mvm::jit::func_llvm_sqrt_f64, args[0], "tmp1", 
                            currentBlock);
  } else if (meth->name == N3::sin) {
    return CallInst::Create(mvm::jit::func_llvm_sin_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::cos) {
    return CallInst::Create(mvm::jit::func_llvm_cos_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::exp) {
    return CallInst::Create(mvm::jit::func_llvm_exp_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::log) {
    return CallInst::Create(mvm::jit::func_llvm_log_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::floor) {
    return CallInst::Create(mvm::jit::func_llvm_floor_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::log10) {
    return CallInst::Create(mvm::jit::func_llvm_log10_f64, args[0], "tmp1",
                            currentBlock);
  } else if (meth->name == N3::pow) {
    Instruction* val = CallInst::Create(mvm::jit::func_llvm_pow_f64, 
                                        args.begin(), args.end(), "tmp1",
                                        currentBlock);
    return val;
  }
  return 0;

}

Instruction* CLIJit::invokeInline(VMMethod* meth, 
                                  std::vector<Value*>& args) {
  
  CLIJit* jit = gc_new(CLIJit)();
  jit->compilingClass = meth->classDef; 
  jit->compilingMethod = meth;
  
  // save current class in case of recursive calls to compile()
  VMGenericClass* old = jit->compilingClass->assembly->currGenericClass;
  // temporarily store the class being compiled in case it is a generic class
  jit->compilingClass->assembly->currGenericClass = dynamic_cast<VMGenericClass*>(jit->compilingClass);
  
  
  jit->unifiedUnreachable = unifiedUnreachable;
  jit->inlineMethods = inlineMethods;
  jit->inlineMethods[meth] = true;
  Instruction* ret = jit->inlineCompile(llvmFunction, currentBlock, 
                                        currentExceptionBlock, args);
  inlineMethods[meth] = false;
  
  // restore saved class
  jit->compilingClass = old;
  
  return ret;
}


void CLIJit::invoke(uint32 value) {
  VMMethod* meth = compilingClass->assembly->getMethodFromToken(value);

  if (meth->classDef->isArray) {
    uint8 func = 0;
    VirtualMachine* vm = VMThread::get()->vm;
    if (meth->name == vm->asciizConstructUTF8("Set")) {
      func = 0;
    } else if (meth->name == vm->asciizConstructUTF8("Get")) {
      func = 1;
    } else if (meth->name == vm->asciizConstructUTF8("Address")) {
      func = 2;
    } else {
      vm->error("implement me %s", meth->name->printString());
    }
      
    VMClassArray* type = (VMClassArray*)meth->classDef;
    uint32 dims = type->dims;
    Value** args = (Value**)alloca(sizeof(Value*) * dims);
    Value* val = 0;
    if (func == 0) {
      val = pop();
    }
    for (sint32 i = dims - 1; i >= 0 ; --i) {
      args[i] = pop();
    }
    Value* obj = pop();
    VMClassArray* base = type;
    for (uint32 v = 0; v < dims; ++v) {
      std::vector<Value*> Args;
      Args.push_back(mvm::jit::constantZero);
      Args.push_back(mvm::jit::constantTwo);
      Args.push_back(args[v]);
      obj = verifyAndComputePtr(obj, args[v], base->naturalType, true);
      if (v != dims - 1) {
        base = (VMClassArray*)base->baseClass;
        obj = new LoadInst(obj, "", currentBlock);
      }
    }
    
    if (func == 0) {
      new StoreInst(val, obj, false, currentBlock);
    } else if (func == 1) {
      push(new LoadInst(obj, "", currentBlock));
    } else {
      push(obj);
    }
    return;
  }

  std::vector<Value*> Args;
  const llvm::FunctionType* type = meth->getSignature();
  makeArgs(type, Args, meth->structReturn);
  
  if (meth->classDef->nameSpace == N3::system && 
      meth->classDef->name == N3::math) {
    Value* val = lowerMathOps(meth, Args); 
    if (val) {
      push(val);
      return;
    }
  } else if (meth->classDef->nameSpace == N3::system && 
             meth->classDef->name == N3::doubleName) {
    if (meth->name == N3::isNan) {
      push(new FCmpInst(FCmpInst::FCMP_UNO, Args[0], 
                        mvm::jit::constantDoubleZero, "tmp1", currentBlock));
      return;
    } else if (meth->name == N3::testInfinity) {
      BasicBlock* endBlock = createBasicBlock("end test infinity");
      BasicBlock* minusInfinity = createBasicBlock("- infinity");
      BasicBlock* noInfinity = createBasicBlock("no infinity");
      PHINode* node = PHINode::Create(Type::Int32Ty, "", endBlock);
      node->addIncoming(mvm::jit::constantOne, currentBlock);
      node->addIncoming(mvm::jit::constantMinusOne, minusInfinity);
      node->addIncoming(mvm::jit::constantZero, noInfinity);
      Value* val1 = new FCmpInst(FCmpInst::FCMP_OEQ, Args[0],
                                 mvm::jit::constantDoubleInfinity, "tmp1",
                                 currentBlock); 
      BranchInst::Create(endBlock, minusInfinity, val1, currentBlock);
      Value* val2 = new FCmpInst(FCmpInst::FCMP_OEQ, Args[0], 
                                 mvm::jit::constantDoubleMinusInfinity, "tmp1",
                                 minusInfinity); 
      BranchInst::Create(endBlock, noInfinity, val2, minusInfinity);
      BranchInst::Create(endBlock, noInfinity);
      currentBlock = endBlock; 
      push(node);
      return;
    }
  } else if (meth->classDef->nameSpace == N3::system && 
             meth->classDef->name == N3::floatName) {
    if (meth->name == N3::isNan) {
      push(new FCmpInst(FCmpInst::FCMP_UNO, Args[0], 
                        mvm::jit::constantFloatZero, "tmp1", currentBlock));
      return;
    } else if (meth->name == N3::testInfinity) {
      BasicBlock* endBlock = createBasicBlock("end test infinity");
      BasicBlock* minusInfinity = createBasicBlock("- infinity");
      BasicBlock* noInfinity = createBasicBlock("no infinity");
      PHINode* node = PHINode::Create(Type::Int32Ty, "", endBlock);
      node->addIncoming(mvm::jit::constantOne, currentBlock);
      node->addIncoming(mvm::jit::constantMinusOne, minusInfinity);
      node->addIncoming(mvm::jit::constantZero, noInfinity);
      Value* val1 = new FCmpInst(FCmpInst::FCMP_OEQ, Args[0], 
                                 mvm::jit::constantFloatInfinity, "tmp1",
                                 currentBlock);
      BranchInst::Create(endBlock, minusInfinity, val1, currentBlock);
      Value* val2 = new FCmpInst(FCmpInst::FCMP_OEQ, Args[0], 
                                 mvm::jit::constantFloatMinusInfinity, "tmp1",
                                 minusInfinity);
      BranchInst::Create(endBlock, noInfinity, val2, minusInfinity);
      BranchInst::Create(endBlock, noInfinity);
      currentBlock = endBlock; 
      push(node);
      return;
    }
  }
  
  Value* res = 0;
  if (meth && meth->canBeInlined && meth != compilingMethod && 
      inlineMethods[meth] == 0) {
    res = invokeInline(meth, Args);
  } else {
    Function* func = meth->compiledPtr();
    
    res = invoke(func, Args, "", currentBlock, meth->structReturn);
  }
  if (meth->parameters[0] != N3::pVoid) {
    push(res);
  }
}

void CLIJit::invokeNew(uint32 value) {
  Assembly* ass = compilingClass->assembly;
  VMMethod* meth = ass->getMethodFromToken(value);
  VMClass* type = meth->classDef;
  const FunctionType* funcType = meth->getSignature();
    
  Value* obj = 0;
  if (type->isPointer) {
    VMThread::get()->vm->error("implement me %s", type->printString());
  } else if (type->isArray) {
    VMClassArray* arrayType = (VMClassArray*)type;
    Value* valCl = new LoadInst(arrayType->llvmVar(), "", currentBlock);
    Value** args = (Value**)alloca(sizeof(Value*) * (arrayType->dims + 1));
    args[0] = valCl;

    for (int cur = arrayType->dims; cur > 0; --cur)
      args[cur] = pop();
     
    std::vector<Value*> Args;
    for (uint32 v = 0; v < arrayType->dims + 1; ++v) {
      Args.push_back(args[v]);
    }
    push(invoke(arrayMultiConsLLVM, Args, "", currentBlock, false));
    return;

  } else if (type->super == N3::pValue || type->super == N3::pEnum) {
    obj = new AllocaInst(type->naturalType, "", currentBlock);
    uint64 size = mvm::jit::getTypeSize(type->naturalType);
        
    std::vector<Value*> params;
    params.push_back(new BitCastInst(obj, mvm::jit::ptrType, "", currentBlock));
    params.push_back(mvm::jit::constantInt8Zero);
    params.push_back(ConstantInt::get(Type::Int32Ty, size));
    params.push_back(mvm::jit::constantZero);
    CallInst::Create(mvm::jit::llvm_memset_i32, params.begin(), params.end(),
                     "", currentBlock);
  } else {
    Value* var = new LoadInst(type->llvmVar(), "", currentBlock);
    Value* val = CallInst::Create(objConsLLVM, var, "", currentBlock);
    obj = new BitCastInst(val, type->naturalType, "", currentBlock);
  }
  
  std::vector<Value*>::iterator i = stack.end();
  uint32 nbParams = funcType->getNumParams();
  while (--nbParams) --i;
  stack.insert(i, obj);

  std::vector<Value*> Args;
  makeArgs(funcType, Args, meth->structReturn);
  if (meth && meth->canBeInlined && meth != compilingMethod && 
      inlineMethods[meth] == 0) {
    invokeInline(meth, Args);
  } else {
    Function* func = meth->compiledPtr();
    
    invoke(func, Args, "", currentBlock, meth->structReturn);
  }
    
  if ((type->super == N3::pValue || type->super == N3::pEnum) &&
      type->virtualFields.size() == 1) {
    push(new LoadInst(obj, "", currentBlock));
  } else {
    push(obj);
  }
}
  
llvm::Value* CLIJit::getVirtualField(uint32 value) {
  VMField* field = compilingClass->assembly->getFieldFromToken(value, false);
  Value* obj = pop();
  if ((field->classDef->super == N3::pValue ||
      field->classDef->super == N3::pEnum) &&
      field->classDef->virtualFields.size() == 1){
    // struct!
    return obj;
  } else {
    if (field->classDef->super != N3::pValue && 
        field->classDef->super != N3::pEnum) {
      obj = new BitCastInst(obj, field->classDef->naturalType, "",
                            currentBlock);
    }
    std::vector<Value*> args;
    args.push_back(mvm::jit::constantZero);
    args.push_back(field->offset);
    Value* ptr = GetElementPtrInst::Create(obj, args.begin(), args.end(), "", 
                                           currentBlock);
    return ptr;
  }
}

llvm::Value* CLIJit::getStaticField(uint32 value) {
  VMField* field = compilingClass->assembly->getFieldFromToken(value, true);
  VMCommonClass* cl = field->classDef;
  cl->resolveType(true, false);
  Value* arg = new LoadInst(cl->llvmVar(), "", currentBlock);
  Value* call = invoke(initialiseClassLLVM, arg, "", currentBlock, false);
  Value* staticCl = new BitCastInst(call, cl->staticType, "", currentBlock);
  
  std::vector<Value*> args;
  args.push_back(mvm::jit::constantZero);
  args.push_back(field->offset);
  Value* ptr = GetElementPtrInst::Create(staticCl, args.begin(), args.end(), "",
                                         currentBlock);
  
  return ptr;

}
  
void CLIJit::setVirtualField(uint32 value) {
  VMField* field = compilingClass->assembly->getFieldFromToken(value, false);
  Value* val = pop();
  Value* obj = pop();
  const Type* valType = val->getType();

  Value* ptr = 0;
  const Type* type = obj->getType();
  if ((field->classDef->super == N3::pValue ||
      field->classDef->super == N3::pEnum) &&
      field->classDef->virtualFields.size() == 1){
    // struct!
    ptr = obj;
  } else {
    if (field->classDef->super != N3::pValue &&
        field->classDef->super != N3::pEnum) {
      obj = new BitCastInst(obj, field->classDef->naturalType, "", currentBlock);
    }
    std::vector<Value*> args;
    args.push_back(mvm::jit::constantZero);
    args.push_back(field->offset);
    ptr = GetElementPtrInst::Create(obj, args.begin(), args.end(), "",
                                currentBlock);
  }

  type = field->signature->naturalType;
  if (val == constantVMObjectNull) {
    val = Constant::getNullValue(type);
  } else if (type != valType) {
    val = changeType(val, type);
  }
  
  new StoreInst(val, ptr, false, currentBlock);
}

void CLIJit::setStaticField(uint32 value) {
  VMField* field = compilingClass->assembly->getFieldFromToken(value, true);
  
  VMCommonClass* cl = field->classDef;
  Value* arg = new LoadInst(cl->llvmVar(), "", currentBlock);
  Value* call = invoke(initialiseClassLLVM, arg, "", currentBlock, false);
  Value* staticCl = new BitCastInst(call, cl->staticType, "", currentBlock);
  
  std::vector<Value*> args;
  args.push_back(mvm::jit::constantZero);
  args.push_back(field->offset);
  Value* ptr = GetElementPtrInst::Create(staticCl, args.begin(), args.end(), "",
                                     currentBlock);
  Value* val = pop();
  const Type* type = field->signature->naturalType;
  const Type* valType = val->getType();
  if (val == constantVMObjectNull) {
    val = Constant::getNullValue(type);
  } else if (type != valType) {
    val = changeType(val, type);
  }
  new StoreInst(val, ptr, false, currentBlock);
}

void CLIJit::JITVerifyNull(Value* obj) {
  CLIJit* jit = this;
  Constant* zero = Constant::getNullValue(obj->getType());
  Value* test = new ICmpInst(ICmpInst::ICMP_EQ, obj, zero, "",
                             jit->currentBlock);

  BasicBlock* exit = jit->createBasicBlock("verifyNullExit");
  BasicBlock* cont = jit->createBasicBlock("verifyNullCont");

  BranchInst::Create(exit, cont, test, jit->currentBlock);
  
  std::vector<Value*> args;
  if (currentExceptionBlock != endExceptionBlock) {
    InvokeInst::Create(CLIJit::nullPointerExceptionLLVM, unifiedUnreachable,
                   currentExceptionBlock, args.begin(),
                   args.end(), "", exit);
  } else {
    CallInst::Create(CLIJit::nullPointerExceptionLLVM, args.begin(),
                 args.end(), "", exit);
    new UnreachableInst(exit);
  }


  jit->currentBlock = cont;
}

llvm::Value* CLIJit::verifyAndComputePtr(llvm::Value* obj, llvm::Value* index,
                                         const llvm::Type* arrayType,
                                         bool verif) {
  JITVerifyNull(obj);
  
  if (index->getType() != Type::Int32Ty) {
    index = changeType(index, Type::Int32Ty);
  }
  
  if (true) {
    Value* size = arraySize(obj);
    
    Value* cmp = new ICmpInst(ICmpInst::ICMP_SLT, index, size, "",
                              currentBlock);

    BasicBlock* ifTrue =  createBasicBlock("true verifyAndComputePtr");
    BasicBlock* ifFalse = createBasicBlock("false verifyAndComputePtr");

    branch(cmp, ifTrue, ifFalse, currentBlock);
    
    std::vector<Value*>args;
    args.push_back(new BitCastInst(obj, VMObject::llvmType, "", ifFalse));
    args.push_back(index);
    if (currentExceptionBlock != endExceptionBlock) {
      InvokeInst::Create(CLIJit::indexOutOfBoundsExceptionLLVM,
                         unifiedUnreachable,
                         currentExceptionBlock, args.begin(),
                         args.end(), "", ifFalse);
    } else {
      CallInst::Create(CLIJit::indexOutOfBoundsExceptionLLVM, args.begin(),
                   args.end(), "", ifFalse);
      new UnreachableInst(ifFalse);
    }
  
    currentBlock = ifTrue;
  }
  
  Constant* zero = mvm::jit::constantZero;
  Value* val = new BitCastInst(obj, arrayType, "", currentBlock);
  
  std::vector<Value*> indexes; //[3];
  indexes.push_back(zero);
  indexes.push_back(VMArray::elementsOffset());
  indexes.push_back(index);
  Value* ptr = GetElementPtrInst::Create(val, indexes.begin(), indexes.end(), 
                                         "", currentBlock);

  return ptr;

}

ConstantInt* VMArray::sizeOffset() {
  return mvm::jit::constantOne;
}

ConstantInt* VMArray::elementsOffset() {
  return mvm::jit::constantTwo;
}

Value* CLIJit::arraySize(Value* array) {
  if (array->getType() != VMArray::llvmType) {
    array = new BitCastInst(array, VMArray::llvmType, "", currentBlock);
  }
  return CallInst::Create(arrayLengthLLVM, array, "", currentBlock);
  /*
  std::vector<Value*> args; //size=  2
  args.push_back(mvm::jit::constantZero);
  args.push_back(VMArray::sizeOffset());
  Value* ptr = GetElementPtrInst::Create(array, args.begin(), args.end(),
                                     "", currentBlock);
  return new LoadInst(ptr, "", currentBlock);*/
}

Function* CLIJit::createDelegate() {
  Function* func = llvmFunction = compilingMethod->methPtr;
  Function::arg_iterator i = func->arg_begin()++, e = func->arg_end();
  Value* obj = i++;
  Value* target = i++;
  Value* handle = i++;
  assert(i == e);
  
  BasicBlock* entry = createBasicBlock("entry");
  obj = new BitCastInst(obj, N3::pDelegate->virtualType, "", entry);
  std::vector<Value*> elts;
  elts.push_back(mvm::jit::constantZero);
  elts.push_back(mvm::jit::constantOne);
  Value* targetPtr = GetElementPtrInst::Create(obj, elts.begin(), elts.end(),
                                               "", entry);
  
  elts.pop_back();
  elts.push_back(mvm::jit::constantTwo);
  Value* handlePtr = GetElementPtrInst::Create(obj, elts.begin(), elts.end(),
                                               "", entry);

  new StoreInst(target, targetPtr, false, entry); 
  new StoreInst(handle, handlePtr, false, entry);
  ReturnInst::Create(entry);
  
  return func;
}
Function* CLIJit::invokeDelegate() {
  VMThread::get()->vm->error("implement me");
  return 0;
}

Function* CLIJit::compileIntern() {
  PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "intern compile %s\n",
              compilingMethod->printString());

  if (compilingClass->subclassOf(N3::pDelegate)) {
    const UTF8* name = compilingMethod->name;
    if (name == N3::ctorName) return createDelegate();
    else if (name == N3::invokeName) return invokeDelegate();
    else VMThread::get()->vm->error("implement me");
  } else {
    VMThread::get()->vm->error("implement me %s",
                               compilingClass->printString());
  }
  return 0;
}

Function* CLIJit::compileNative() {
  PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "native compile %s\n",
              compilingMethod->printString());
    
  const FunctionType *funcType = compilingMethod->getSignature();
  
  Function* func = llvmFunction = compilingMethod->methPtr;
  currentBlock = createBasicBlock("start");
  // TODO
  /*endExceptionBlock = createBasicBlock("exceptionBlock");
  endBlock = createBasicBlock("end");
  new UnwindInst(endExceptionBlock);*/
  
  uint32 nargs = func->arg_size();
  std::vector<Value*> nativeArgs;
  
  uint32 index = 0;
  for (Function::arg_iterator i = func->arg_begin(); 
       index < nargs; ++i, ++index) {   
    nativeArgs.push_back(i);
  }

  void* natPtr = NativeUtil::nativeLookup(compilingClass, compilingMethod);
  
  Value* valPtr = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, (uint64)natPtr),
                              PointerType::getUnqual(funcType));
  
  Value* result = CallInst::Create(valPtr, nativeArgs.begin(),
                                   nativeArgs.end(), "", currentBlock);
  
  
  if (result->getType() != Type::VoidTy)
    ReturnInst::Create(result, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  
  return llvmFunction;
}

uint32 CLIJit::readExceptionTable(uint32 offset, bool fat) {
  Assembly* ass = compilingClass->assembly;
  ArrayUInt8* bytes = ass->bytes;
  uint32 nbe = 0;
  if (fat) {
    nbe = (READ_U3(bytes, offset) - 4) / 24;
  } else {
    nbe = (READ_U1(bytes, offset) - 4) / 12;
    READ_U2(bytes, offset);
  }
  
   if (nbe) {
    supplLocal = new AllocaInst(VMObject::llvmType, "exceptionVar",
                                currentBlock);
  }
  
  BasicBlock* realEndExceptionBlock = endExceptionBlock;
  // TODO synchronized

  for (uint32 i = 0; i < nbe; ++i) {
    Exception* ex = gc_new(Exception)();
    uint32 flags = 0;
    uint32 classToken = 0;
    if (fat) {
      flags = READ_U4(bytes, offset);
      ex->tryOffset = READ_U4(bytes, offset);
      ex->tryLength = READ_U4(bytes, offset);
      ex->handlerOffset = READ_U4(bytes, offset);
      ex->handlerLength = READ_U4(bytes, offset);
      classToken = READ_U4(bytes, offset);
    } else {
      flags = READ_U2(bytes, offset);
      ex->tryOffset = READ_U2(bytes, offset);
      ex->tryLength = READ_U1(bytes, offset);
      ex->handlerOffset = READ_U2(bytes, offset);
      ex->handlerLength = READ_U1(bytes, offset);
      classToken = READ_U4(bytes, offset);
    }
    
    if (!(opcodeInfos[ex->handlerOffset].newBlock)) {
      opcodeInfos[ex->handlerOffset].newBlock = 
                              createBasicBlock("handlerException");
    }
    
    ex->handler = opcodeInfos[ex->handlerOffset].newBlock;
    
    if (flags == CONSTANT_COR_ILEXCEPTION_CLAUSE_EXCEPTION) {
      ex->test = createBasicBlock("testException");
      if (classToken) {
        ex->catchClass = ass->loadType((N3*)VMThread::get()->vm, classToken,
                                       true, false, false, true);
      } else {
        ex->catchClass = N3::pException;
      }
      opcodeInfos[ex->handlerOffset].reqSuppl = true;
      exceptions.push_back(ex);

      for (uint16 i = ex->tryOffset; i < ex->tryOffset + ex->tryLength; ++i) {
        if (opcodeInfos[i].exceptionBlock == endExceptionBlock) {
          opcodeInfos[i].exceptionBlock = ex->test;
        }    
      }  

    } else if (flags == CONSTANT_COR_ILEXCEPTION_CLAUSE_FINALLY) {
      ex->catchClass = 0;
      finallyHandlers.push_back(ex);
    } else {
      VMThread::get()->vm->error("implement me");
    }
  }
  
  bool first = true;
  for (std::vector<Exception*>::iterator i = exceptions.begin(),
    e = exceptions.end(); i!= e; ++i) {

    Exception* cur = *i;
    Exception* next = 0;
    if (i + 1 != e) {
      next = *(i + 1);
    }

    if (first) {
      cur->realTest = createBasicBlock("realTestException");
    } else {
      cur->realTest = cur->test;
    }
    
    if (next && cur->tryOffset == next->tryOffset && 
        cur->tryOffset + cur->tryLength == next->tryOffset + next->tryLength)
      first = false;
    else
      first = true;
      
  }

  for (std::vector<Exception*>::iterator i = exceptions.begin(),
    e = exceptions.end(); i!= e; ++i) {

    Exception* cur = *i;
    Exception* next = 0;
    BasicBlock* bbNext = 0;
    if (i + 1 != e) {
      next = *(i + 1);
      if (cur->tryOffset >= next->tryOffset && 
          cur->tryOffset + cur->tryLength <=
            next->tryOffset + next->tryLength) {
        bbNext = realEndExceptionBlock;
      } else {
        bbNext = next->realTest;
      }
    } else {
      bbNext = realEndExceptionBlock;
    }

    if (cur->realTest != cur->test) {
      const PointerType* PointerTy_0 = mvm::jit::ptrType;
      std::vector<Value*> int32_eh_select_params;
      Instruction* ptr_eh_ptr = CallInst::Create(mvm::jit::llvmGetException,
                                                 "eh_ptr", cur->test);
      int32_eh_select_params.push_back(ptr_eh_ptr);
      Constant* C = ConstantExpr::getCast(Instruction::BitCast,
                                          mvm::jit::personality, PointerTy_0);
      int32_eh_select_params.push_back(C);
      int32_eh_select_params.push_back(mvm::jit::constantPtrNull);
      CallInst::Create(mvm::jit::exceptionSelector,
                       int32_eh_select_params.begin(),
                       int32_eh_select_params.end(), "eh_select", cur->test);
      BranchInst::Create(cur->realTest, cur->test);
    } 

    Value* cl = new LoadInst(cur->catchClass->llvmVar(), "", cur->realTest);
    Value* cmp = CallInst::Create(compareExceptionLLVM, cl, "", cur->realTest);
    BranchInst::Create(cur->handler, bbNext, cmp, cur->realTest);
    
    if (cur->handler->empty()) {
      Value* cpp = CallInst::Create(getCppExceptionLLVM, "", cur->handler);
      Value* exc = CallInst::Create(getCLIExceptionLLVM, "", cur->handler);
      CallInst::Create(clearExceptionLLVM, "", cur->handler);
      CallInst::Create(mvm::jit::exceptionBeginCatch, cpp, "tmp8",
                       cur->handler);
      std::vector<Value*> void_28_params;
      CallInst::Create(mvm::jit::exceptionEndCatch, void_28_params.begin(),
                       void_28_params.end(), "", cur->handler);
      new StoreInst(exc, supplLocal, false, cur->handler);
      
      for (uint16 i = cur->tryOffset; i < cur->tryOffset + cur->tryLength;
           ++i) {
        opcodeInfos[i].exception = exc;
      }  
    } 
     
  }

  return nbe;

}

Function* CLIJit::compileFatOrTiny() {
  PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "tiny or fat compile %s\n",
              compilingMethod->printString());
  uint32 offset = compilingMethod->offset;
  ArrayUInt8* bytes = compilingClass->assembly->bytes;
  uint8 header = READ_U1(bytes, offset);
  bool tiny = false;
  uint32 localVarSig = 0;
  uint32 maxStack = 0;
  uint32 codeLen = 0;
  uint32 nbe = 0;

  if ((header & 3) == CONSTANT_CorILMethod_TinyFormat) {
    tiny = true;
    codeLen = (header & 0xfffc) >> 2;
  } else if ((header & 3) != CONSTANT_CorILMethod_FatFormat) {
    VMThread::get()->vm->error("unknown Method Format");
  } else {
    header += (READ_U1(bytes, offset) << 8); //header
    maxStack = READ_U2(bytes, offset);
    codeLen = READ_U4(bytes, offset);
    localVarSig = READ_U4(bytes, offset);
  }
    

  /* TODO Synchronize
  bool synchro = isSynchro(compilingMethod->flags);
  */

  const FunctionType *funcType = compilingMethod->getSignature();
  
  Function* func = llvmFunction = compilingMethod->methPtr;
  currentBlock = createBasicBlock("start");
  endExceptionBlock = createBasicBlock("exceptionBlock");
  unifiedUnreachable = createBasicBlock("unifiedUnreachable"); 
  

  opcodeInfos = (Opinfo*)alloca(codeLen * sizeof(Opinfo));
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExceptionBlock;
  }
  
  if (!tiny) {
    if (header & CONSTANT_CorILMethod_MoreSects) {
      uint32 excpOffset = 0;
      if ((codeLen % 4) == 0) {
        excpOffset = offset + codeLen;
      } else {
        excpOffset = offset + codeLen + (4 - (codeLen % 4));
      }

      uint8 flags = READ_U1(bytes, excpOffset);
      nbe = readExceptionTable(excpOffset, 
                         flags & CONSTANT_CorILMethod_Sect_FatFormat);
    }
  }

  for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end(); 
       i != e; ++i) {
    
    const Type* cur = i->getType();

    AllocaInst* alloc = new AllocaInst(cur, "", currentBlock);
    new StoreInst(i, alloc, false, currentBlock);
    arguments.push_back(alloc);
  } 
  
  if (localVarSig) {
    std::vector<VMCommonClass*> temp;
    compilingClass->assembly->readSignature(localVarSig, temp);

    for (std::vector<VMCommonClass*>::iterator i = temp.begin(), 
            e = temp.end(); i!= e; ++i) {
      VMCommonClass* cl = *i;
      cl->resolveType(false, false);
      AllocaInst* alloc = new AllocaInst(cl->naturalType, "", currentBlock);
      if (cl->naturalType->isSingleValueType()) {
        new StoreInst(Constant::getNullValue(cl->naturalType), alloc, false,
                      currentBlock);
      } else {
        uint64 size = mvm::jit::getTypeSize(cl->naturalType);
        
        std::vector<Value*> params;
        params.push_back(new BitCastInst(alloc, mvm::jit::ptrType, "",
                                         currentBlock));
        params.push_back(mvm::jit::constantInt8Zero);
        params.push_back(ConstantInt::get(Type::Int32Ty, size));
        params.push_back(mvm::jit::constantZero);
        CallInst::Create(mvm::jit::llvm_memset_i32, params.begin(),
                         params.end(), "", currentBlock);

      }
      locals.push_back(alloc);
    }
  }

  exploreOpcodes(&compilingClass->assembly->bytes->elements[offset], codeLen); 
 
  endBlock = createBasicBlock("end");

  const Type* endType = funcType->getReturnType(); 
  if (endType != Type::VoidTy) {
    endNode = PHINode::Create(endType, "", endBlock);
  } else if (compilingMethod->structReturn) {
    const Type* lastType = 
      funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    endNode = PHINode::Create(lastType, "", endBlock);
  }

  compileOpcodes(&compilingClass->assembly->bytes->elements[offset], codeLen);
  
  currentBlock = endBlock;
  pred_iterator PI = pred_begin(endBlock);
  pred_iterator PE = pred_end(endBlock);
  if (PI == PE) {
    endBlock->eraseFromParent();
  } else {
    if (endType != Type::VoidTy) {
      ReturnInst::Create(endNode, endBlock);
    } else if (compilingMethod->structReturn) {
      const Type* lastType = 
        funcType->getContainedType(funcType->getNumContainedTypes() - 1);
      uint64 size = mvm::jit::getTypeSize(lastType->getContainedType(0));
      Value* obj = --llvmFunction->arg_end();
      std::vector<Value*> params;
      params.push_back(new BitCastInst(obj, mvm::jit::ptrType, "",
                                       currentBlock));
      params.push_back(new BitCastInst(endNode, mvm::jit::ptrType, "",
                                       currentBlock));
      params.push_back(ConstantInt::get(Type::Int32Ty, size));
      params.push_back(mvm::jit::constantFour);
      CallInst::Create(mvm::jit::llvm_memcpy_i32, params.begin(), params.end(),
                       "", currentBlock);
      ReturnInst::Create(currentBlock);
    } else {
      ReturnInst::Create(endBlock);
    }
  }
  
  PI = pred_begin(endExceptionBlock);
  PE = pred_end(endExceptionBlock);
  if (PI == PE) {
    endExceptionBlock->eraseFromParent();
  } else {
    CallInst* ptr_eh_ptr = CallInst::Create(getCppExceptionLLVM, "eh_ptr", 
                                        endExceptionBlock);
    CallInst::Create(mvm::jit::unwindResume, ptr_eh_ptr, "", endExceptionBlock);
    new UnreachableInst(endExceptionBlock);
  }
  
  PI = pred_begin(unifiedUnreachable);
  PE = pred_end(unifiedUnreachable);
  if (PI == PE) {
    unifiedUnreachable->eraseFromParent();
  } else {
    new UnreachableInst(unifiedUnreachable);
  }
  
  mvm::jit::runPasses(llvmFunction, VMThread::get()->perFunctionPasses);
  
  if (nbe == 0 && codeLen < 50) {
    PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "%s can be inlined\n",
                compilingMethod->printString());
    compilingMethod->canBeInlined = true;
  }
  
  return llvmFunction;
}

Instruction* CLIJit::inlineCompile(Function* parentFunction, BasicBlock*& curBB,
                                   BasicBlock* endExBlock,
                                   std::vector<Value*>& args) {
  
  PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL, "tiny or fat inline compile %s\n",
              compilingMethod->printString());
  uint32 offset = compilingMethod->offset;
  ArrayUInt8* bytes = compilingClass->assembly->bytes;
  uint8 header = READ_U1(bytes, offset);
  bool tiny = false;
  uint32 localVarSig = 0;
  uint32 maxStack = 0;
  uint32 codeLen = 0;
  
  if ((header & 3) == CONSTANT_CorILMethod_TinyFormat) {
    tiny = true;
    codeLen = (header & 0xfffc) >> 2;
  } else if ((header & 3) != CONSTANT_CorILMethod_FatFormat) {
    VMThread::get()->vm->error("unknown Method Format");
  } else {
    header += (READ_U1(bytes, offset) << 8); //header
    maxStack = READ_U2(bytes, offset);
    codeLen = READ_U4(bytes, offset);
    localVarSig = READ_U4(bytes, offset);
  }
    

  /* TODO Synchronize
  bool synchro = isSynchro(compilingMethod->flags);
  */

  const FunctionType *funcType = compilingMethod->getSignature();
  
  llvmFunction = parentFunction;
  currentBlock = curBB;
  endExceptionBlock = 0; 
  

  opcodeInfos = (Opinfo*)alloca(codeLen * sizeof(Opinfo));
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExBlock;
  }
  
  if (!tiny) {
    if (header & CONSTANT_CorILMethod_MoreSects) {
      assert(0 && "inlining a function with exceptions!");
      uint32 excpOffset = 0;
      if ((codeLen % 4) == 0) {
        excpOffset = offset + codeLen;
      } else {
        excpOffset = offset + codeLen + (4 - (codeLen % 4));
      }

      uint8 flags = READ_U1(bytes, excpOffset);
      readExceptionTable(excpOffset, 
                         flags & CONSTANT_CorILMethod_Sect_FatFormat);
    }
  }

  for (std::vector<Value*>::iterator i = args.begin(), e = args.end(); 
       i != e; ++i) {
    
    const Type* cur = (*i)->getType();

    AllocaInst* alloc = new AllocaInst(cur, "", currentBlock);
    new StoreInst(*i, alloc, false, currentBlock);
    arguments.push_back(alloc);
  } 

  if (localVarSig) {
    std::vector<VMCommonClass*> temp;
    compilingClass->assembly->readSignature(localVarSig, temp);

    for (std::vector<VMCommonClass*>::iterator i = temp.begin(), 
            e = temp.end(); i!= e; ++i) {
      VMCommonClass* cl = *i;
      cl->resolveType(false, false);
      AllocaInst* alloc = new AllocaInst(cl->naturalType, "", currentBlock);
      if (cl->naturalType->isSingleValueType()) {
        new StoreInst(Constant::getNullValue(cl->naturalType), alloc, false,
                      currentBlock);
      } else {
        uint64 size = mvm::jit::getTypeSize(cl->naturalType);
        
        std::vector<Value*> params;
        params.push_back(new BitCastInst(alloc, mvm::jit::ptrType, "",
                                         currentBlock));
        params.push_back(mvm::jit::constantInt8Zero);
        params.push_back(ConstantInt::get(Type::Int32Ty, size));
        params.push_back(mvm::jit::constantZero);
        CallInst::Create(mvm::jit::llvm_memset_i32, params.begin(),
                         params.end(), "", currentBlock);

      }
      locals.push_back(alloc);
    }
  }

  exploreOpcodes(&compilingClass->assembly->bytes->elements[offset], codeLen); 
 
  endBlock = createBasicBlock("end");

  const Type* endType = funcType->getReturnType(); 
  if (endType != Type::VoidTy) {
    endNode = PHINode::Create(endType, "", endBlock);
  } else if (compilingMethod->structReturn) {
    const Type* lastType = 
      funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    endNode = PHINode::Create(lastType, "", endBlock);
  }

  compileOpcodes(&compilingClass->assembly->bytes->elements[offset], codeLen);
  
  curBB = endBlock;
  
  PRINT_DEBUG(N3_COMPILE, 1, COLOR_NORMAL,
              "end tiny or fat inline compile %s\n",
              compilingMethod->printString());
  
  return endNode;
}


Function* CLIJit::compile(VMClass* cl, VMMethod* meth) {
  CLIJit* jit = gc_new(CLIJit)();
  jit->compilingClass = cl; 
  jit->compilingMethod = meth;

  // save current class in case of recursive calls to compile()
  VMGenericClass* old = cl->assembly->currGenericClass;
  // temporarily store the class being compiled in case it is a generic class
  cl->assembly->currGenericClass = dynamic_cast<VMGenericClass*>(cl);
  
  meth->getSignature();
  if (isInternal(meth->implFlags)) {
    return jit->compileNative();
  } else if (meth->offset == 0) {
    return jit->compileIntern();
  } else {
    return jit->compileFatOrTiny();
  }
  
  // restore saved class
  cl->assembly->currGenericClass = old;
}

llvm::Function *VMMethod::compiledPtr() {
  if (methPtr != 0) return methPtr;
  else {
    classDef->aquire();
    if (methPtr == 0) {
      methPtr = Function::Create(getSignature(), GlobalValue::GhostLinkage,
                                   printString(), classDef->vm->module);
      classDef->vm->functions->hash(methPtr, this);
    }
    classDef->release();
    return methPtr;
  }
}

void VMField::initField(VMObject* obj) {
  VMField* field = this;
  ConstantInt* offset = field->offset;
  const TargetData* targetData = mvm::jit::executionEngine->getTargetData();
  bool stat = isStatic(field->flags);
  const Type* clType = stat ? field->classDef->staticType :
                              field->classDef->virtualType;
  
  const StructLayout* sl =
    targetData->getStructLayout((StructType*)(clType->getContainedType(0)));
  uint64 ptrOffset = sl->getElementOffset(offset->getZExtValue());
  
  field->ptrOffset = ptrOffset;

}

extern "C" VMObject* initialiseClass(VMClass* cl) {
  cl->clinitClass();
  return cl->staticInstance;
}

extern "C" void classCastException() {
  fflush(stdout);
  assert(0 && "implement class cast exception");
}

extern "C" void nullPointerException() {
  fflush(stdout);
  assert(0 && "implement null pointer exception");
}

extern "C" void indexOutOfBounds() {
  fflush(stdout);
  assert(0 && "implement index out of bounds exception");
}


extern "C" VMObject* newString(const UTF8* utf8) {
  CLIString * str = 
    (CLIString*)(((N3*)VMThread::get()->vm)->UTF8ToStr(utf8));
  return str;
}

void CLIJit::initialise() {
}

void CLIJit::initialiseAppDomain(N3* vm) {
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider);
  mvm::jit::protectEngine->unlock();
}

void CLIJit::initialiseBootstrapVM(N3* vm) {
  Module* module = vm->module;
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider);
  mvm::jit::protectEngine->unlock();
 
  

  {
    std::vector<const llvm::Type *> arg_types;
    arg_types.insert (arg_types.begin (), llvm::PointerType::getUnqual(mvm::jit::ptrType));
    
    llvm::FunctionType *mtype = llvm::FunctionType::get (llvm::Type::VoidTy, arg_types, false);
    Function::Create(mtype,  llvm::GlobalValue::ExternalLinkage, "llvm.va_start", module);
  }

  {
    std::vector<const llvm::Type *> arg_types;
    arg_types.insert (arg_types.begin (), llvm::Type::Int32Ty);
  
    llvm::FunctionType *mtype = llvm::FunctionType::get (llvm::PointerType::getUnqual(llvm::Type::Int8Ty), arg_types, false);
    Function::Create(mtype,  llvm::GlobalValue::ExternalLinkage, "llvm.frameaddress", module);
  }

  
  {
     const llvm::Type *BPTy = PointerType::getUnqual(llvm::Type::Int8Ty);
     // Prototype malloc as "char* malloc(...)", because we don't know in
     // doInitialization whether size_t is int or long.
     FunctionType *FT = FunctionType::get(BPTy, std::vector<const llvm::Type*>(), true);
     Function::Create(FT, llvm::GlobalValue::ExternalLinkage, "_ZN2gcnwEjP5gc_vt", module); 
  }


  // Create VMObject::llvmType
  const llvm::Type* Pty = llvm::PointerType::getUnqual(llvm::Type::Int8Ty);
  
  std::vector<const llvm::Type*> objectFields;
  objectFields.push_back(Pty); // VT
  objectFields.push_back(Pty); // Class
  objectFields.push_back(Pty); // Lock
  VMObject::llvmType = 
    llvm::PointerType::getUnqual(llvm::StructType::get(objectFields, false));

  // Create VMArray::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(VMObject::llvmType->getContainedType(0));
  arrayFields.push_back(llvm::Type::Int32Ty);
  VMArray::llvmType =
    llvm::PointerType::getUnqual(llvm::StructType::get(arrayFields, false));
  }

#define ARRAY_TYPE(name, type)                                            \
  {                                                                       \
  std::vector<const Type*> arrayFields;                                   \
  arrayFields.push_back(VMObject::llvmType->getContainedType(0));       \
  arrayFields.push_back(Type::Int32Ty);                                   \
  arrayFields.push_back(ArrayType::get(type, 0));                         \
  name::llvmType = PointerType::getUnqual(StructType::get(arrayFields, false)); \
  }

  ARRAY_TYPE(ArrayUInt8, Type::Int8Ty);
  ARRAY_TYPE(ArraySInt8, Type::Int8Ty);
  ARRAY_TYPE(ArrayUInt16, Type::Int16Ty);
  ARRAY_TYPE(ArraySInt16, Type::Int16Ty);
  ARRAY_TYPE(ArrayUInt32, Type::Int32Ty);
  ARRAY_TYPE(ArraySInt32, Type::Int32Ty);
  ARRAY_TYPE(ArrayLong, Type::Int64Ty);
  ARRAY_TYPE(ArrayDouble, Type::DoubleTy);
  ARRAY_TYPE(ArrayFloat, Type::FloatTy);
  ARRAY_TYPE(ArrayObject, VMObject::llvmType);

#undef ARRAY_TYPE

  // Create UTF8::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(VMObject::llvmType->getContainedType(0));
  arrayFields.push_back(llvm::Type::Int32Ty);
  arrayFields.push_back(llvm::ArrayType::get(llvm::Type::Int16Ty, 0));
  UTF8::llvmType =
    llvm::PointerType::getUnqual(llvm::StructType::get(arrayFields, false));
  }
  
  // Create CacheNode::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // VT
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // methPtr
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // lastCible
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // next
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // enveloppe
  arrayFields.push_back(Type::Int1Ty); // box
  CacheNode::llvmType =
    PointerType::getUnqual(StructType::get(arrayFields, false));
  }
  
  // Create Enveloppe::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // VT
  arrayFields.push_back(CacheNode::llvmType); // firstCache
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // cacheLock
  arrayFields.push_back(PointerType::getUnqual(Type::Int8Ty)); // originalMethod
  Enveloppe::llvmType =
    PointerType::getUnqual(StructType::get(arrayFields, false));
  }

#ifdef WITH_TRACER
  // Create markAndTraceLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
#ifdef MULTIPLE_GC
  args.push_back(mvm::jit::ptrType);
#endif
  markAndTraceLLVMType = FunctionType::get(llvm::Type::VoidTy, args, false);
  markAndTraceLLVM = Function::Create(markAndTraceLLVMType,
                                  GlobalValue::ExternalLinkage,
#ifdef MULTIPLE_GC
                                  "_ZNK2gc12markAndTraceEP9Collector",
#else
                                  "_ZNK2gc12markAndTraceEv",
#endif
                                  module);
  }
#endif
  
  // Create vmObjectTracerLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
#ifdef MULTIPLE_GC
  args.push_back(mvm::jit::ptrType);
#endif
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);
  vmObjectTracerLLVM = Function::Create(type,
                                    GlobalValue::ExternalLinkage,
#ifdef MULTIPLE_GC
                                    "_ZN2n38VMObject6tracerEPv",
#else
                                    "_ZN2n38VMObject6tracerEv",
#endif
                                    module);
  }

  // Create intialiseClassLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args, false);
  initialiseClassLLVM = Function::Create(type,
                                     GlobalValue::ExternalLinkage,
                                     "initialiseClass",
                                     module);
  }

  
  // Create resolveStringLLVM
  /*{
  std::vector<const Type*> args;
  args.push_back(UTF8::llvmType);
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  args.push_back(llvm::Type::Int32Ty);
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);
  stringLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                "_ZN5n37CLIJit12stringLookupEPKNS_4UTF8EPNS_5ClassEj",
                module);
  }*/
 
  // Create staticLookupLLVM
  /*{
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  args.push_back(llvm::Type::Int32Ty);
  const FunctionType* type =
    FunctionType::get(llvm::PointerType::getUnqual(llvm::Type::Int8Ty), args,
                      false);

  staticLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n37CLIJit12staticLookupEPNS_5ClassEj",
                     module);
  }*/
  
  // Create virtualLookupLLVM
  {
  std::vector<const Type*> args;
  //args.push_back(VMObject::llvmType);
  //args.push_back(PointerType::getUnqual(Type::Int8Ty));
  //args.push_back(llvm::Type::Int32Ty);
  args.push_back(CacheNode::llvmType);
  args.push_back(VMObject::llvmType);
  const FunctionType* type =
    FunctionType::get(CacheNode::llvmType, args, false);

  virtualLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                                   "virtualLookup", module);

/*
  virtualLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n37CLIJit13virtualLookupEPNS_10VMObjectEPNS_5ClassEj",
                     module);
*/
  }
  
  // Create newLookupLLVM
  /*{
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  args.push_back(llvm::Type::Int32Ty);
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);

  newLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n37CLIJit9newLookupEPNS_5ClassEj",
                     module);
  }*/
  
  // Create arrayConsLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);

  arrayConsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n312VMClassArray5doNewEj",
                     module);
  }
  
  // Create objConsLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);

  objConsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n37VMClass5doNewEv",
                     module);
  }
  
  // Create objInitLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  args.push_back(VMObject::llvmType);
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);

  objInitLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n37VMClass16initialiseObjectEPNS_8VMObjectE",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  objInitLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create arrayLengthLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMArray::llvmType);
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args, false);

  arrayLengthLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "arrayLength",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  arrayLengthLLVM->setParamAttrs(func_toto_PAL);
  }

  
  // Create nullPointerExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  nullPointerExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "nullPointerException",
                     module);
  }
  
  // Create classCastExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  classCastExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "classCastException",
                     module);
  }
  
  // Create indexOutOfBoundsExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  indexOutOfBoundsExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "indexOutOfBounds",
                     module);
  }
  
  // Create proceedPendingExceptionLLVM
  /*{
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  jniProceedPendingExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n37CLIJit26jniProceedPendingExceptionEv",
                     module);
  }*/
  
  // Create printExecutionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  printExecutionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n36CLIJit14printExecutionEPcPNS_8VMMethodE",
                     module);
  }
  
  // Create throwExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  throwExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMThread14throwExceptionEPNS_8VMObjectE",
                     module);
  }
   
  // Create clearExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  clearExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMThread14clearExceptionEv",
                     module);
  }
  
  
  // Create compareExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(Type::Int1Ty, args, false);

  compareExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMThread16compareExceptionEPNS_7VMClassE",
                     module);
  }
  
  // Create instanceOfLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args, false);

  instanceOfLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMObject10instanceOfEPNS_13VMCommonClassE",
                     module);
  }
  
  // Create arrayMultiConsLLVM
  {
  std::vector<const Type*> args;
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               true);

  arrayMultiConsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "doMultiNew",
                     module);
  }

  /*
  // Create aquireObjectLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  aquireObjectLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n310VMObject6aquireEv",
                     module);
  }
  
  // Create releaseObjectLLVM
  {
  std::vector<const Type*> args;
  args.push_back(VMObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  releaseObjectLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n310VMObject6unlockEv",
                     module);
  }
  
  */
  
  
  // Create *AconsLLVM
  /*{
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  args.push_back(PointerType::getUnqual(Type::Int8Ty));
  const FunctionType* type = FunctionType::get(VMObject::llvmType, args,
                                               false);

  FloatAconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n310ArrayFloat5aconsEiPNS_10VMClassArrayE",
                     module);
  
  Int8AconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n310ArraySInt85aconsEiPNS_10VMClassArrayE",
                     module);
  
  DoubleAconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n311ArrayDouble5aconsEiPNS_10VMClassArrayE",
                     module);
   
  Int16AconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n311ArraySInt165aconsEiPNS_10VMClassArrayE",
                     module);
  
  Int32AconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n311ArraySInt325aconsEiPNS_10VMClassArrayE",
                     module);
  
  UTF8AconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n34UTF85aconsEiPNS_10VMClassArrayE",
                     module);
  
  LongAconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n39ArrayLong5aconsEiPNS_10VMClassArrayE",
                     module);
  
  ObjectAconsLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5n311ArrayObject5aconsEiPNS_10VMClassArrayE",
                     module);
  }*/
  
  // Create getCppExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, 
                                               args, false);

  getCppExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMThread15getCppExceptionEv",
                     module);
  }
  
  // Create getCLIExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(VMObject::llvmType, 
                                               args, false);

  getCLIExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN2n38VMThread15getCLIExceptionEv",
                     module);
  }
  
  // Create newStringLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(VMObject::llvmType, 
                                               args, false);

  newStringLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "newString",
                     module);
  }

  
  constantVMObjectNull = Constant::getNullValue(VMObject::llvmType);
}

Constant* CLIJit::constantVMObjectNull;


#if N3_EXECUTE > 1
static void printArgs(std::vector<llvm::Value*> args, BasicBlock* insertAt) {
  for (std::vector<llvm::Value*>::iterator i = args.begin(),
       e = args.end(); i!= e; ++i) {
    llvm::Value* arg = *i;
    const llvm::Type* type = arg->getType();
    if (type == Type::Int8Ty || type == Type::Int16Ty || type == Type::Int1Ty) {
      CallInst::Create(mvm::jit::printIntLLVM, new ZExtInst(arg, Type::Int32Ty, "", insertAt), "", insertAt);
    } else if (type == Type::Int32Ty) {
      CallInst::Create(mvm::jit::printIntLLVM, arg, "", insertAt);
    } else if (type == Type::Int64Ty) {
      CallInst::Create(mvm::jit::printLongLLVM, arg, "", insertAt);
    } else if (type == Type::FloatTy) {
      CallInst::Create(mvm::jit::printFloatLLVM, arg, "", insertAt);
    } else if (type == Type::DoubleTy) {
      CallInst::Create(mvm::jit::printDoubleLLVM, arg, "", insertAt);
    } else {
      CallInst::Create(mvm::jit::printObjectLLVM, new BitCastInst(arg, mvm::jit::ptrType, "", insertAt), "", insertAt);
    }
  }

}
#endif

Value* CLIJit::invoke(Value *F, std::vector<llvm::Value*> args,
                       const char* Name,
                       BasicBlock *InsertAtEnd, bool structReturn) {
#if N3_EXECUTE > 1
  printArgs(args, InsertAtEnd);
#endif

  Value* ret = 0;
  if (structReturn) {
    const Type* funcType = F->getType();
    if (isa<PointerType>(funcType)) {
      funcType = funcType->getContainedType(0);
    }
    const Type* lastType = funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    ret = new AllocaInst(lastType->getContainedType(0), "", InsertAtEnd);
    args.push_back(ret);
  }
  Value* val = 0;
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    val = InvokeInst::Create(F, ifNormal, currentExceptionBlock, args.begin(), 
                         args.end(), Name, InsertAtEnd);
  } else {
    val = CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
  if (ret) return ret;
  else return val;
}

Value* CLIJit::invoke(Value *F, Value* arg1, const char* Name,
                       BasicBlock *InsertAtEnd, bool structReturn) {
  
  std::vector<Value*> args;
  args.push_back(arg1);
#if N3_EXECUTE > 1
  printArgs(args, InsertAtEnd);
#endif
  Value* ret = 0;
  if (structReturn) {
    const Type* funcType = F->getType();
    if (isa<PointerType>(funcType)) {
      funcType = funcType->getContainedType(0);
    }
    const Type* lastType = funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    ret = new AllocaInst(lastType->getContainedType(0), "", InsertAtEnd);
    args.push_back(ret);
  }
  
  Value* val = 0;
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    val = InvokeInst::Create(F, ifNormal, currentExceptionBlock, args.begin(),
                                args.end(), Name, InsertAtEnd);
  } else {
    val = CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
  
  if (ret) return ret;
  else return val; 
}

Value* CLIJit::invoke(Value *F, Value* arg1, Value* arg2,
                       const char* Name, BasicBlock *InsertAtEnd, 
                       bool structReturn) {
  
  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
#if N3_EXECUTE > 1
  printArgs(args, InsertAtEnd);
#endif
  
  Value* ret = 0;
  if (structReturn) {
    const Type* funcType = F->getType();
    if (isa<PointerType>(funcType)) {
      funcType = funcType->getContainedType(0);
    }
    const Type* lastType = funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    ret = new AllocaInst(lastType->getContainedType(0), "", InsertAtEnd);
    args.push_back(ret);
  }

  Value* val = 0;
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    val = InvokeInst::Create(F, ifNormal, currentExceptionBlock, args.begin(),
                         args.end(), Name, InsertAtEnd);
  } else {
    val = CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
  if (ret) return ret;
  else return val; 
}

Value* CLIJit::invoke(Value *F, const char* Name,
                       BasicBlock *InsertAtEnd, bool structReturn) {
  
  std::vector<Value*> args;
  Value* ret = 0;
  if (structReturn) {
    const Type* funcType = F->getType();
    if (isa<PointerType>(funcType)) {
      funcType = funcType->getContainedType(0);
    }
    const Type* lastType = funcType->getContainedType(funcType->getNumContainedTypes() - 1);
    ret = new AllocaInst(lastType->getContainedType(0), "", InsertAtEnd);
    args.push_back(ret);
  }

  Value* val = 0;
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    val = InvokeInst::Create(F, ifNormal, currentExceptionBlock, args.begin(),
                         args.end(), Name, InsertAtEnd);
  } else {
    val = CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
  if (ret) return ret;
  else return val; 
}




namespace mvm {
llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*, llvm::Function*);
llvm::FunctionPass* createLowerArrayLengthPass();
}


static void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}

void AddStandardCompilePasses(FunctionPassManager *PM) {
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  // LLVM does not allow calling functions from other modules in verifier
  //PM->add(llvm::createVerifierPass());                  // Verify that input is correct
  
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up disgusting code
  addPass(PM, llvm::createScalarReplAggregatesPass());// Kill useless allocas
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE
  addPass(PM, llvm::createPromoteMemoryToRegisterPass());// Kill useless allocas
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE
  
  addPass(PM, llvm::createTailDuplicationPass());      // Simplify cfg by copying code
  addPass(PM, llvm::createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, llvm::createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, llvm::createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, llvm::createInstructionCombiningPass()); // Combine silly seq's
  addPass(PM, llvm::createCondPropagationPass());      // Propagate conditionals
  
   
  addPass(PM, llvm::createTailCallEliminationPass());  // Eliminate tail calls
  addPass(PM, llvm::createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, llvm::createReassociatePass());          // Reassociate expressions
  addPass(PM, llvm::createLoopRotatePass());
  addPass(PM, llvm::createLICMPass());                 // Hoist loop invariants
  addPass(PM, llvm::createLoopUnswitchPass());         // Unswitch loops.
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after LICM/reassoc
  addPass(PM, llvm::createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, llvm::createLoopUnrollPass());           // Unroll small loops
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after the unroller
  //addPass(PM, mvm::createArrayChecksPass()); 
  addPass(PM, llvm::createGVNPass());                  // GVN for load instructions
  addPass(PM, llvm::createGCSEPass());                 // Remove common subexprs
  addPass(PM, llvm::createSCCPPass());                 // Constant prop with SCCP
  addPass(PM, llvm::createPredicateSimplifierPass());                
  
  
  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, llvm::createInstructionCombiningPass());
  addPass(PM, llvm::createCondPropagationPass());      // Propagate conditionals

  addPass(PM, llvm::createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, llvm::createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
  addPass(PM, llvm::createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, mvm::createLowerArrayLengthPass());
}
