//===----------- JavaJIT.cpp - Java just in time compiler -----------------===//
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
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/CFG.h>

#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "debug.h"
#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaJIT.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "NativeUtil.h"
#include "Reader.h"
#include "Zip.h"

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;
using namespace llvm;

BasicBlock* JavaJIT::createBasicBlock(const char* name) {
  return llvm::BasicBlock::Create(name, llvmFunction);
}

Value* JavaJIT::top() {
  return stack.back().first;
}

Value* JavaJIT::pop() {
  llvm::Value * ret = top();
  stack.pop_back();
  return ret;
}

Value* JavaJIT::popAsInt() {
  llvm::Value * ret = top();
  const AssessorDesc* ass = topFunc();
  stack.pop_back();

  if (ret->getType() != Type::Int32Ty) {
    if (ass == AssessorDesc::dChar) {
      ret = new ZExtInst(ret, Type::Int32Ty, "", currentBlock);
    } else {
      ret = new SExtInst(ret, Type::Int32Ty, "", currentBlock);
    }
  }

  return ret;
}

void JavaJIT::push(llvm::Value* val, const AssessorDesc* ass) {
  assert(ass->llvmType == val->getType());
  stack.push_back(std::make_pair(val, ass));
}

void JavaJIT::push(std::pair<llvm::Value*, const AssessorDesc*> pair) {
  assert(pair.second->llvmType == pair.first->getType());
  stack.push_back(pair);
}


const AssessorDesc* JavaJIT::topFunc() {
  return stack.back().second;
}

uint32 JavaJIT::stackSize() {
  return stack.size();
}
  
void JavaJIT::invokeVirtual(uint16 index) {
  
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  CommonClass* cl = 0;
  JavaMethod* meth = 0;
  ctpInfo->infoOfMethod(index, ACC_VIRTUAL, cl, meth);
  
  if ((cl && isFinal(cl->access)) || 
      (meth && (isFinal(meth->access) || isPrivate(meth->access))))
    return invokeSpecial(index);
 

#ifndef WITHOUT_VTABLE
  Constant* zero = mvm::jit::constantZero;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index);
  std::vector<Value*> args; // size = [signature->nbIn + 3];

  FunctionType::param_iterator it  = signature->virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  
  JITVerifyNull(args[0]); 

  std::vector<Value*> indexes; //[3];
  indexes.push_back(zero);
  indexes.push_back(zero);
  Value* VTPtr = GetElementPtrInst::Create(args[0], indexes.begin(),
                                           indexes.end(), "", currentBlock);
    
  Value* VT = new LoadInst(VTPtr, "", currentBlock);
  std::vector<Value*> indexes2; //[3];
  if (meth) {
    indexes2.push_back(meth->offset);
  } else {
    compilingClass->isolate->protectModule->lock();
    GlobalVariable* gv = 
      new GlobalVariable(Type::Int32Ty, false, GlobalValue::ExternalLinkage,
                         zero, "", compilingClass->isolate->module);
    compilingClass->isolate->protectModule->unlock();
    
    // set is volatile
    Value* val = new LoadInst(gv, "", true, currentBlock);
    Value * cmp = new ICmpInst(ICmpInst::ICMP_NE, val, zero, "", currentBlock);
    BasicBlock* ifTrue  = createBasicBlock("true vtable");
    BasicBlock* ifFalse  = createBasicBlock("false vtable");
    BasicBlock* endBlock  = createBasicBlock("end vtable");
    PHINode * node = llvm::PHINode::Create(Type::Int32Ty, "", endBlock);
    llvm::BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
    
    currentBlock = ifTrue;
    node->addIncoming(val, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);

    currentBlock = ifFalse;
    std::vector<Value*> Args;
    Args.push_back(args[0]);
    Module* M = compilingClass->isolate->module;
    Args.push_back(new LoadInst(compilingClass->llvmVar(M), "", currentBlock));
    mvm::jit::protectConstants();//->lock();
    Constant* CI = ConstantInt::get(Type::Int32Ty, index);
    Args.push_back(CI);
    mvm::jit::unprotectConstants();//->unlock();
    Args.push_back(gv);
    val = invoke(vtableLookupLLVM, Args, "", currentBlock);
    node->addIncoming(val, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);
    
    currentBlock = endBlock;
    indexes2.push_back(node);
  }
  
  Value* FuncPtr = GetElementPtrInst::Create(VT, indexes2.begin(),
                                             indexes2.end(), "",
                                             currentBlock);
    
  Value* Func = new LoadInst(FuncPtr, "", currentBlock);
  Func = new BitCastInst(Func, signature->virtualTypePtr, "", currentBlock);

  Value* val = invoke(Func, args, "", currentBlock);
  
  const llvm::Type* retType = signature->virtualType->getReturnType();
  if (retType != Type::VoidTy) {
    push(val, signature->ret->funcs);
    if (retType == Type::DoubleTy || retType == Type::Int64Ty) {
      push(mvm::jit::constantZero, AssessorDesc::dInt);
    }
  }
    
#else
  return invokeInterfaceOrVirtual(index);
#endif
}

std::pair<llvm::Value*, const AssessorDesc*> JavaJIT::popPair() {
  std::pair<Value*, const AssessorDesc*> ret = stack.back();
  stack.pop_back();
  return ret;
}

llvm::Function* JavaJIT::nativeCompile(void* natPtr) {
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "native compile %s\n",
              compilingMethod->printString());
  
  bool stat = isStatic(compilingMethod->access);

  const FunctionType *funcType = compilingMethod->llvmType;
  
  bool jnjvm = false;
  natPtr = natPtr ? natPtr :
              NativeUtil::nativeLookup(compilingClass, compilingMethod, jnjvm);
  
  
  
  Function* func = llvmFunction = compilingMethod->llvmFunction;
  if (jnjvm) {
    mvm::jit::executionEngine->addGlobalMapping(func, natPtr);
    return llvmFunction;
  }
  
  currentExceptionBlock = endExceptionBlock = 0;
  currentBlock = createBasicBlock("start");
  BasicBlock* executeBlock = createBasicBlock("execute");
  endBlock = createBasicBlock("end block");

#if defined(MULTIPLE_VM)
  Value* lastArg = 0;
  for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end();
       i != e; ++i) {
    lastArg = i;
  }
#if !defined(SERVICE_VM)
  isolateLocal = lastArg;
#else
  if (compilingClass->isolate == Jnjvm::bootstrapVM) {
    isolateLocal = lastArg;
  } else {
    JavaObject* loader = compilingClass->classLoader;
    ServiceDomain* vm = ServiceDomain::getDomainFromLoader(loader);
    isolateLocal = new LoadInst(vm->llvmDelegatee(), "", currentBlock);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, lastArg, 
                              isolateLocal, "", currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(lastArg);
    Args.push_back(isolateLocal);
    CallInst::Create(ServiceDomain::serviceCallStartLLVM, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
  }
#endif
#endif

  if (funcType->getReturnType() != Type::VoidTy)
    endNode = llvm::PHINode::Create(funcType->getReturnType(), "", endBlock);
  
  Value* buf = llvm::CallInst::Create(getSJLJBufferLLVM, "", currentBlock);
  Value* test = llvm::CallInst::Create(mvm::jit::setjmpLLVM, buf, "", currentBlock);
  test = new ICmpInst(ICmpInst::ICMP_EQ, test, mvm::jit::constantZero, "",
                      currentBlock);
  llvm::BranchInst::Create(executeBlock, endBlock, test, currentBlock);

  if (compilingMethod->signature->ret->funcs != AssessorDesc::dVoid) {
    Constant* C = compilingMethod->signature->ret->funcs->llvmNullConstant;
    endNode->addIncoming(C, currentBlock);
  }
  
  currentBlock = executeBlock;
  if (isSynchro(compilingMethod->access))
    beginSynchronize();
  
  
  uint32 nargs = func->arg_size() + 1 + (stat ? 1 : 0); 
  std::vector<Value*> nativeArgs;
  
  mvm::jit::protectConstants();
  int64_t jniEnv = (int64_t)&(compilingClass->isolate->jniEnv);
  nativeArgs.push_back(
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, jniEnv), 
                              mvm::jit::ptrType));
  mvm::jit::unprotectConstants();

  uint32 index = 0;
  if (stat) {
    Module* M = compilingClass->isolate->module;
    nativeArgs.push_back(compilingClass->llvmDelegatee(M, currentBlock));
    index = 2;
  } else {
    index = 1;
  }
  for (Function::arg_iterator i = func->arg_begin(); 
       index < nargs; ++i, ++index) {
     
    nativeArgs.push_back(i);
  }
  
  
  const llvm::Type* valPtrType = compilingMethod->signature->nativeTypePtr;
  mvm::jit::protectConstants();//->lock();
  Value* valPtr = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, (uint64)natPtr),
                              valPtrType);
  mvm::jit::unprotectConstants();//->unlock();

  Value* result = llvm::CallInst::Create(valPtr, nativeArgs.begin(), nativeArgs.end(), 
                               "", currentBlock);

  if (funcType->getReturnType() != Type::VoidTy)
    endNode->addIncoming(result, currentBlock);
  llvm::BranchInst::Create(endBlock, currentBlock);

  currentBlock = endBlock; 
  if (isSynchro(compilingMethod->access))
    endSynchronize();
  
  llvm::CallInst::Create(jniProceedPendingExceptionLLVM, "", currentBlock);
  
  if (funcType->getReturnType() != Type::VoidTy)
    llvm::ReturnInst::Create(result, currentBlock);
  else
    llvm::ReturnInst::Create(currentBlock);
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "end native compile %s\n",
              compilingMethod->printString());
  
  func->setLinkage(GlobalValue::ExternalLinkage);
  
  return llvmFunction;
}

void JavaJIT::beginSynchronize() {
  std::vector<Value*> argsSync;
  if (isVirtual(compilingMethod->access)) {
    argsSync.push_back(llvmFunction->arg_begin());
  } else {
    Value* arg = compilingClass->staticVar(this);
    argsSync.push_back(arg);
  }
#ifdef SERVICE_VM
  if (ServiceDomain::isLockableDomain(compilingClass->isolate))
    llvm::CallInst::Create(aquireObjectInSharedDomainLLVM, argsSync.begin(),
                           argsSync.end(), "", currentBlock);
  else
#endif
  llvm::CallInst::Create(aquireObjectLLVM, argsSync.begin(), argsSync.end(),
                         "", currentBlock);
}

void JavaJIT::endSynchronize() {
  std::vector<Value*> argsSync;
  if (isVirtual(compilingMethod->access)) {
    argsSync.push_back(llvmFunction->arg_begin());
  } else {
    Value* arg = compilingClass->staticVar(this);
    argsSync.push_back(arg);
  }
#ifdef SERVICE_VM
  if (ServiceDomain::isLockableDomain(compilingClass->isolate))
    llvm::CallInst::Create(releaseObjectInSharedDomainLLVM, argsSync.begin(),
                           argsSync.end(), "", currentBlock);
  else
#endif
  llvm::CallInst::Create(releaseObjectLLVM, argsSync.begin(), argsSync.end(), "",
               currentBlock);    
}


Instruction* JavaJIT::inlineCompile(Function* parentFunction, 
                                    BasicBlock*& curBB,
                                    BasicBlock* endExBlock,
                                    std::vector<Value*>& args) {
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "inline compile %s\n",
              compilingMethod->printString());


  Attribut* codeAtt = Attribut::lookup(&compilingMethod->attributs,
                                       Attribut::codeAttribut);
  
  if (!codeAtt) {
    Jnjvm* vm = JavaThread::get()->isolate;
    vm->unknownError("unable to find the code attribut in %s",
                     compilingMethod->printString());
  }

  Reader* reader = codeAtt->toReader(compilingClass->isolate,
                                     compilingClass->bytes, codeAtt);
  maxStack = reader->readU2();
  maxLocals = reader->readU2();
  codeLen = reader->readU4();
  uint32 start = reader->cursor;
  
  reader->seek(codeLen, Reader::SeekCur);
  
  const FunctionType *funcType = compilingMethod->llvmType;
  returnType = funcType->getReturnType();
  endBlock = createBasicBlock("end");

  llvmFunction = parentFunction;
  currentBlock = curBB;
  endExceptionBlock = 0;
  
  opcodeInfos = (Opinfo*)alloca(codeLen * sizeof(Opinfo));
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExBlock;
  }
  
  for (int i = 0; i < maxLocals; i++) {
    intLocals.push_back(new AllocaInst(Type::Int32Ty, "", currentBlock));
    doubleLocals.push_back(new AllocaInst(Type::DoubleTy, "", currentBlock));
    longLocals.push_back(new AllocaInst(Type::Int64Ty, "", currentBlock));
    floatLocals.push_back(new AllocaInst(Type::FloatTy, "", currentBlock));
    objectLocals.push_back(new AllocaInst(JavaObject::llvmType, "",
                                          currentBlock));
  }
  
  uint32 index = 0;
  uint32 count = 0;
#ifdef MULTIPLE_VM
  uint32 max = args.size() - 1;
#else
  uint32 max = args.size();
#endif
  for (std::vector<Value*>::iterator i = args.begin();
       count < max; ++i, ++index, ++count) {
    
    const Type* cur = (*i)->getType();

    if (cur == Type::Int64Ty ){
      new StoreInst(*i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur == Type::Int8Ty || cur == Type::Int16Ty) {
      new StoreInst(new ZExtInst(*i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (cur == Type::Int32Ty) {
      new StoreInst(*i, intLocals[index], false, currentBlock);
    } else if (cur == Type::DoubleTy) {
      new StoreInst(*i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (cur == Type::FloatTy) {
      new StoreInst(*i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(*i, objectLocals[index], false, currentBlock);
    }
  }

#if defined(MULTIPLE_VM)
#if !defined(SERVICE_VM)
  isolateLocal = args[args.size() - 1];
#else
  if (compilingClass->isolate == Jnjvm::bootstrapVM) {
    isolateLocal = args[args.size() - 1];
  } else {
    JavaObject* loader = compilingClass->classLoader;
    ServiceDomain* vm = ServiceDomain::getDomainFromLoader(loader);
    isolateLocal = new LoadInst(vm->llvmDelegatee(), "", currentBlock);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, args[args.size() - 1], 
                              isolateLocal, "", currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(args[args.size()-  1]);
    Args.push_back(isolateLocal);
    CallInst::Create(ServiceDomain::serviceCallStartLLVM, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
  }
#endif
#endif
  
  exploreOpcodes(&compilingClass->bytes->elements[start], codeLen);
  

  if (returnType != Type::VoidTy) {
    endNode = llvm::PHINode::Create(returnType, "", endBlock);
  }

  compileOpcodes(&compilingClass->bytes->elements[start], codeLen);
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "--> end inline compiling %s\n",
              compilingMethod->printString());

#if defined(SERVICE_VM)
  if (compilingClass->isolate != Jnjvm::bootstrapVM) {
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, args[args.size() - 1], 
                              isolateLocal, "", currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* newEndBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, newEndBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(args[args.size() - 1]);
    Args.push_back(isolateLocal);
    CallInst::Create(ServiceDomain::serviceCallStopLLVM, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(newEndBlock, currentBlock);
    currentBlock = newEndBlock;
    endBlock = currentBlock;
  }
#endif
  
  curBB = endBlock;
  return endNode;
    
}


llvm::Function* JavaJIT::javaCompile() {
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "compiling %s\n",
              compilingMethod->printString());


  Attribut* codeAtt = Attribut::lookup(&compilingMethod->attributs,
                                       Attribut::codeAttribut);
  
  if (!codeAtt) {
    Jnjvm* vm = JavaThread::get()->isolate;
    vm->unknownError("unable to find the code attribut in %s",
                     compilingMethod->printString());
  }

  Reader* reader = codeAtt->toReader(compilingClass->isolate,
                                     compilingClass->bytes, codeAtt);
  maxStack = reader->readU2();
  maxLocals = reader->readU2();
  codeLen = reader->readU4();
  uint32 start = reader->cursor;
  
  reader->seek(codeLen, Reader::SeekCur);

  const FunctionType *funcType = compilingMethod->llvmType;
  returnType = funcType->getReturnType();
  
  Function* func = llvmFunction = compilingMethod->llvmFunction;

  currentBlock = createBasicBlock("start");
  endExceptionBlock = createBasicBlock("endExceptionBlock");
  unifiedUnreachable = createBasicBlock("unifiedUnreachable"); 

  opcodeInfos = (Opinfo*)alloca(codeLen * sizeof(Opinfo));
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExceptionBlock;
  }
    
#if JNJVM_EXECUTE > 0
    {
    std::vector<llvm::Value*> args;
    mvm::jit::protectConstants();//->lock();
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
    mvm::jit::unprotectConstants();//->unlock();
    llvm::CallInst::Create(printMethodStartLLVM, args.begin(), args.end(), "",
                           currentBlock);
    }
#endif

  

  for (int i = 0; i < maxLocals; i++) {
    intLocals.push_back(new AllocaInst(Type::Int32Ty, "", currentBlock));
    doubleLocals.push_back(new AllocaInst(Type::DoubleTy, "", currentBlock));
    longLocals.push_back(new AllocaInst(Type::Int64Ty, "", currentBlock));
    floatLocals.push_back(new AllocaInst(Type::FloatTy, "", currentBlock));
    objectLocals.push_back(new AllocaInst(JavaObject::llvmType, "",
                                          currentBlock));
  }
  
  uint32 index = 0;
  uint32 count = 0;
#ifdef MULTIPLE_VM
  uint32 max = func->arg_size() - 1;
#else
  uint32 max = func->arg_size();
#endif
  Function::arg_iterator i = func->arg_begin(); 
  for (;count < max; ++i, ++index, ++count) {
    
    const Type* cur = i->getType();

    if (cur == Type::Int64Ty ){
      new StoreInst(i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur == Type::Int8Ty || cur == Type::Int16Ty) {
      new StoreInst(new ZExtInst(i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (cur == Type::Int32Ty) {
      new StoreInst(i, intLocals[index], false, currentBlock);
    } else if (cur == Type::DoubleTy) {
      new StoreInst(i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (cur == Type::FloatTy) {
      new StoreInst(i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(i, objectLocals[index], false, currentBlock);
    }
  }

#if defined(MULTIPLE_VM)
#if !defined(SERVICE_VM)
  isolateLocal = i;
#else
  if (compilingClass->isolate == Jnjvm::bootstrapVM) {
    isolateLocal = i;
  } else {
    JavaObject* loader = compilingClass->classLoader;
    ServiceDomain* vm = ServiceDomain::getDomainFromLoader(loader);
    isolateLocal = new LoadInst(vm->llvmDelegatee(), "", currentBlock);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, i, isolateLocal, "",
                              currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(i);
    Args.push_back(isolateLocal);
    CallInst::Create(ServiceDomain::serviceCallStartLLVM, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
  }
#endif
#endif
  
  unsigned nbe = readExceptionTable(reader);
  
  exploreOpcodes(&compilingClass->bytes->elements[start], codeLen);
  

 
  endBlock = createBasicBlock("end");

  if (returnType != Type::VoidTy) {
    endNode = llvm::PHINode::Create(returnType, "", endBlock);
  }
  
  if (isSynchro(compilingMethod->access))
    beginSynchronize();

  compileOpcodes(&compilingClass->bytes->elements[start], codeLen); 
  currentBlock = endBlock;

  if (isSynchro(compilingMethod->access))
    endSynchronize();

#if JNJVM_EXECUTE > 0
    {
    std::vector<llvm::Value*> args;
    mvm::jit::protectConstants();//->lock();
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
    mvm::jit::unprotectConstants();//->unlock();
    llvm::CallInst::Create(printMethodEndLLVM, args.begin(), args.end(), "",
                           currentBlock);
    }
#endif
  
#if defined(SERVICE_VM)
  if (compilingClass->isolate != Jnjvm::bootstrapVM) {
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, i, isolateLocal, "",
                              currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* newEndBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, newEndBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(i);
    Args.push_back(isolateLocal);
    CallInst::Create(ServiceDomain::serviceCallStopLLVM, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(newEndBlock, currentBlock);
    currentBlock = newEndBlock;
  }
#endif

  if (returnType != Type::VoidTy)
    llvm::ReturnInst::Create(endNode, currentBlock);
  else
    llvm::ReturnInst::Create(currentBlock);

  pred_iterator PI = pred_begin(endExceptionBlock);
  pred_iterator PE = pred_end(endExceptionBlock);
  if (PI == PE) {
    endExceptionBlock->eraseFromParent();
  } else {
    CallInst* ptr_eh_ptr = llvm::CallInst::Create(getExceptionLLVM, "eh_ptr", 
                                        endExceptionBlock);
    llvm::CallInst::Create(mvm::jit::unwindResume, ptr_eh_ptr, "",
                           endExceptionBlock);
    new UnreachableInst(endExceptionBlock);
  }
  
  PI = pred_begin(unifiedUnreachable);
  PE = pred_end(unifiedUnreachable);
  if (PI == PE) {
    unifiedUnreachable->eraseFromParent();
  } else {
    new UnreachableInst(unifiedUnreachable);
  }
  
  func->setLinkage(GlobalValue::ExternalLinkage);
  mvm::jit::runPasses(llvmFunction, JavaThread::get()->perFunctionPasses);
  
  /*
  if (compilingMethod->name == compilingClass->isolate->asciizConstructUTF8("main")) {
    llvmFunction->print(llvm::cout);
    void* res = mvm::jit::executionEngine->getPointerToGlobal(llvmFunction);
    void* base = res;
    while (base <  (void*)((char*)res + ((mvm::Code*)res)->objectSize())) {
      printf("%08x\t", (unsigned)base);
      int n= mvm::jit::disassemble((unsigned int *)base);
      printf("\n");
      base= ((void *)((char *)base + n));
    }
    printf("\n");
    fflush(stdout);
  }*/
  

  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "--> end compiling %s\n",
              compilingMethod->printString());
  
  if (nbe == 0 && codeLen < 50)
    compilingMethod->canBeInlined = false;

  return llvmFunction;
}


unsigned JavaJIT::readExceptionTable(Reader* reader) {
  BasicBlock* temp = currentBlock;
  uint16 nbe = reader->readU2();
  std::vector<Exception*> exceptions;  
  unsigned sync = isSynchro(compilingMethod->access) ? 1 : 0;
  nbe += sync;
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  if (nbe) {
    supplLocal = new AllocaInst(JavaObject::llvmType, "exceptionVar",
                                currentBlock);
  }
  
  BasicBlock* realEndExceptionBlock = endExceptionBlock;
  currentExceptionBlock = endExceptionBlock;
  if (sync) {
    BasicBlock* synchronizeExceptionBlock = 
          createBasicBlock("synchronizeExceptionBlock");
    BasicBlock* trySynchronizeExceptionBlock = 
          createBasicBlock("trySynchronizeExceptionBlock");
    realEndExceptionBlock = synchronizeExceptionBlock;
    std::vector<Value*> argsSync;
    if (isVirtual(compilingMethod->access)) {
      argsSync.push_back(llvmFunction->arg_begin());
    } else {
      Value* arg = compilingClass->staticVar(this);
      argsSync.push_back(arg);
    }
    llvm::CallInst::Create(releaseObjectLLVM, argsSync.begin(), argsSync.end(),
                           "", synchronizeExceptionBlock);

    llvm::BranchInst::Create(endExceptionBlock, synchronizeExceptionBlock);
    
    const PointerType* PointerTy_0 = mvm::jit::ptrType;
    std::vector<Value*> int32_eh_select_params;
    Instruction* ptr_eh_ptr = 
      llvm::CallInst::Create(mvm::jit::llvmGetException, "eh_ptr",
                             trySynchronizeExceptionBlock);
    int32_eh_select_params.push_back(ptr_eh_ptr);
    Constant* C = ConstantExpr::getCast(Instruction::BitCast,
                                        mvm::jit::personality, PointerTy_0);
    int32_eh_select_params.push_back(C);
    int32_eh_select_params.push_back(mvm::jit::constantPtrNull);
    llvm::CallInst::Create(mvm::jit::exceptionSelector,
                           int32_eh_select_params.begin(), 
                           int32_eh_select_params.end(), 
                           "eh_select", trySynchronizeExceptionBlock);

    llvm::BranchInst::Create(synchronizeExceptionBlock,
                             trySynchronizeExceptionBlock);

    for (uint16 i = 0; i < codeLen; ++i) {
      if (opcodeInfos[i].exceptionBlock == endExceptionBlock) {
        opcodeInfos[i].exceptionBlock = trySynchronizeExceptionBlock;
      }
    }
  }

  for (uint16 i = 0; i < nbe - sync; ++i) {
    Exception* ex = new Exception();
    ex->startpc   = reader->readU2();
    ex->endpc     = reader->readU2();
    ex->handlerpc = reader->readU2();

    ex->catche = reader->readU2();

    if (ex->catche) {
      Class* cl = (Class*)(ctpInfo->getMethodClassIfLoaded(ex->catche));
      ex->catchClass = cl;
    } else {
      ex->catchClass = Classpath::newThrowable;
    }
    
    ex->test = createBasicBlock("testException");
    
    // We can do this because readExceptionTable is the first function to be
    // called after creation of Opinfos
    for (uint16 i = ex->startpc; i < ex->endpc; ++i) {
      if (opcodeInfos[i].exceptionBlock == realEndExceptionBlock) {
        opcodeInfos[i].exceptionBlock = ex->test;
      }
    }

    if (!(opcodeInfos[ex->handlerpc].newBlock)) {
      opcodeInfos[ex->handlerpc].newBlock = 
                    createBasicBlock("handlerException");
    }
    
    ex->handler = opcodeInfos[ex->handlerpc].newBlock;
    opcodeInfos[ex->handlerpc].reqSuppl = true;

    exceptions.push_back(ex);
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
    
    cur->exceptionPHI = llvm::PHINode::Create(mvm::jit::ptrType, "",
                                              cur->realTest);

    if (next && cur->startpc == next->startpc && cur->endpc == next->endpc)
      first = false;
    else
      first = true;
      
  }

  for (std::vector<Exception*>::iterator i = exceptions.begin(),
    e = exceptions.end(); i!= e; ++i) {

    Exception* cur = *i;
    Exception* next = 0;
    BasicBlock* bbNext = 0;
    PHINode* nodeNext = 0;
    currentExceptionBlock = opcodeInfos[cur->handlerpc].exceptionBlock;

    if (i + 1 != e) {
      next = *(i + 1);
      if (!(cur->startpc >= next->startpc && cur->endpc <= next->endpc)) {
        bbNext = realEndExceptionBlock;
      } else {
        bbNext = next->realTest;
        nodeNext = next->exceptionPHI;
      }
    } else {
      bbNext = realEndExceptionBlock;
    }

    if (cur->realTest != cur->test) {
      const PointerType* PointerTy_0 = mvm::jit::ptrType;
      std::vector<Value*> int32_eh_select_params;
      Instruction* ptr_eh_ptr = 
        llvm::CallInst::Create(mvm::jit::llvmGetException, "eh_ptr", cur->test);
      int32_eh_select_params.push_back(ptr_eh_ptr);
      Constant* C = ConstantExpr::getCast(Instruction::BitCast,
                                          mvm::jit::personality, PointerTy_0);
      int32_eh_select_params.push_back(C);
      int32_eh_select_params.push_back(mvm::jit::constantPtrNull);
      llvm::CallInst::Create(mvm::jit::exceptionSelector,
                             int32_eh_select_params.begin(),
                             int32_eh_select_params.end(), "eh_select",
                             cur->test);
      llvm::BranchInst::Create(cur->realTest, cur->test);
      cur->exceptionPHI->addIncoming(ptr_eh_ptr, cur->test);
    } 
    
    Module* M = compilingClass->isolate->module;
    Value* cl = 0;
    currentBlock = cur->realTest;
    if (cur->catchClass)
      cl = new LoadInst(cur->catchClass->llvmVar(M), "", currentBlock);
    else
      cl = getResolvedClass(cur->catche, false);
    Value* cmp = llvm::CallInst::Create(compareExceptionLLVM, cl, "",
                                        currentBlock);
    llvm::BranchInst::Create(cur->handler, bbNext, cmp, currentBlock);
    if (nodeNext)
      nodeNext->addIncoming(cur->exceptionPHI, currentBlock);
    
    if (cur->handler->empty()) {
      cur->handlerPHI = llvm::PHINode::Create(mvm::jit::ptrType, "",
                                              cur->handler);
      cur->handlerPHI->addIncoming(cur->exceptionPHI, currentBlock);
      Value* exc = llvm::CallInst::Create(getJavaExceptionLLVM, "",
                                          cur->handler);
      llvm::CallInst::Create(clearExceptionLLVM, "", cur->handler);
      llvm::CallInst::Create(mvm::jit::exceptionBeginCatch, cur->handlerPHI,
                             "tmp8", cur->handler);
      std::vector<Value*> void_28_params;
      llvm::CallInst::Create(mvm::jit::exceptionEndCatch,
                             void_28_params.begin(), void_28_params.end(), "",
                             cur->handler);
      new StoreInst(exc, supplLocal, false, cur->handler);
    } else {
      Instruction* insn = cur->handler->begin();
      ((PHINode*)insn)->addIncoming(cur->exceptionPHI, currentBlock);
    }
     
  }
  
  for (std::vector<Exception*>::iterator i = exceptions.begin(),
    e = exceptions.end(); i!= e; ++i) {
    delete *i;
  }
  
  currentBlock = temp;
  return nbe;

}

void JavaJIT::compareFP(Value* val1, Value* val2, const Type* ty, bool l) {
  Value* one = mvm::jit::constantOne;
  Value* zero = mvm::jit::constantZero;
  Value* minus = mvm::jit::constantMinusOne;

  Value* c = new FCmpInst(FCmpInst::FCMP_UGT, val1, val2, "", currentBlock);
  Value* r = llvm::SelectInst::Create(c, one, zero, "", currentBlock);
  c = new FCmpInst(FCmpInst::FCMP_ULT, val1, val2, "", currentBlock);
  r = llvm::SelectInst::Create(c, minus, r, "", currentBlock);
  c = new FCmpInst(FCmpInst::FCMP_UNO, val1, val2, "", currentBlock);
  r = llvm::SelectInst::Create(c, l ? one : minus, r, "", currentBlock);

  push(r, AssessorDesc::dInt);

}

void JavaJIT::_ldc(uint16 index) {
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  uint8 type = ctpInfo->typeAt(index);
  
  if (type == JavaCtpInfo::ConstantString) {
    Value* toPush = 0;
    if (ctpInfo->ctpRes[index] == 0) {
      compilingClass->aquire();
      if (ctpInfo->ctpRes[index] == 0) {
        const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[index]);
        void* val = 0;
        GlobalVariable* gv = 0;
#ifndef MULTIPLE_VM
        val = compilingClass->isolate->UTF8ToStr(utf8);
        compilingClass->isolate->protectModule->lock();
        gv =
          new GlobalVariable(JavaObject::llvmType, false, 
                             GlobalValue::ExternalLinkage,
                             constantJavaObjectNull, "",
                             compilingClass->isolate->module);
        compilingClass->isolate->protectModule->unlock();
#else
          val = (void*)utf8;
          compilingClass->isolate->protectModule->lock();
          gv =
            new GlobalVariable(UTF8::llvmType, false, 
                               GlobalValue::ExternalLinkage,
                               constantUTF8Null, "",
                               compilingClass->isolate->module);
          compilingClass->isolate->protectModule->unlock();
#endif
        
        // TODO: put an initialiser in here
        void* ptr = mvm::jit::executionEngine->getPointerToGlobal(gv);
        GenericValue Val = GenericValue(val);
        llvm::GenericValue * Ptr = (llvm::GenericValue*)ptr;
        mvm::jit::executionEngine->StoreValueToMemory(Val, Ptr,
                                                      JavaObject::llvmType);
        toPush = new LoadInst(gv, "", currentBlock);
        ctpInfo->ctpRes[index] = gv;
        compilingClass->release();
      } else {
        compilingClass->release();
        toPush = new LoadInst((GlobalVariable*)ctpInfo->ctpRes[index], "",
                              currentBlock);
      }
    } else {
      toPush = new LoadInst((GlobalVariable*)ctpInfo->ctpRes[index], "",
                            currentBlock);
    }
#ifdef MULTIPLE_VM
    CallInst* C = llvm::CallInst::Create(runtimeUTF8ToStrLLVM, toPush, "",
                                         currentBlock);
    push(C, AssessorDesc::dRef);
#else
    push(toPush, AssessorDesc::dRef);
#endif
  } else if (type == JavaCtpInfo::ConstantLong) {
    mvm::jit::protectConstants();//->lock();
    push(ConstantInt::get(Type::Int64Ty, ctpInfo->LongAt(index)),
         AssessorDesc::dLong);
    mvm::jit::unprotectConstants();//->unlock();
  } else if (type == JavaCtpInfo::ConstantDouble) {
    mvm::jit::protectConstants();//->lock();
    push(ConstantFP::get(Type::DoubleTy, APFloat(ctpInfo->DoubleAt(index))),
         AssessorDesc::dDouble);
    mvm::jit::unprotectConstants();//->unlock();
  } else if (type == JavaCtpInfo::ConstantInteger) {
    mvm::jit::protectConstants();//->lock();
    push(ConstantInt::get(Type::Int32Ty, ctpInfo->IntegerAt(index)),
         AssessorDesc::dInt);
    mvm::jit::unprotectConstants();//->unlock();
  } else if (type == JavaCtpInfo::ConstantFloat) {
    mvm::jit::protectConstants();//->lock();
    push(ConstantFP::get(Type::FloatTy, APFloat(ctpInfo->FloatAt(index))),
         AssessorDesc::dFloat);
    mvm::jit::unprotectConstants();//->unlock();
  } else if (type == JavaCtpInfo::ConstantClass) {
    if (ctpInfo->ctpRes[index]) {
      CommonClass* cl = (CommonClass*)(ctpInfo->ctpRes[index]);
      push(cl->llvmDelegatee(compilingClass->isolate->module, currentBlock),
           AssessorDesc::dRef);
    } else {
      Value* val = getResolvedClass(index, false);
      Value* res = CallInst::Create(getClassDelegateeLLVM, val, "",
                                    currentBlock);
      push(res, AssessorDesc::dRef);
    }
  } else {
    JavaThread::get()->isolate->unknownError("unknown type %d", type);
  }
}

void JavaJIT::JITVerifyNull(Value* obj) { 

  JavaJIT* jit = this;
  Constant* zero = constantJavaObjectNull;
  Value* test = new ICmpInst(ICmpInst::ICMP_EQ, obj, zero, "",
                             jit->currentBlock);

  BasicBlock* exit = jit->createBasicBlock("verifyNullExit");
  BasicBlock* cont = jit->createBasicBlock("verifyNullCont");

  llvm::BranchInst::Create(exit, cont, test, jit->currentBlock);
  std::vector<Value*> args;
  if (currentExceptionBlock != endExceptionBlock) {
    llvm::InvokeInst::Create(JavaJIT::nullPointerExceptionLLVM,
                             unifiedUnreachable,
                             currentExceptionBlock, args.begin(),
                             args.end(), "", exit);
  } else {
    llvm::CallInst::Create(JavaJIT::nullPointerExceptionLLVM, args.begin(),
                 args.end(), "", exit);
    new UnreachableInst(exit);
  }
  

  jit->currentBlock = cont;
  
}

Value* JavaJIT::verifyAndComputePtr(Value* obj, Value* index,
                                    const Type* arrayType, bool verif) {
  JITVerifyNull(obj);
  
  if (index->getType() != Type::Int32Ty) {
    index = new SExtInst(index, Type::Int32Ty, "", currentBlock);
  }
  
  if (true) {
    Value* size = arraySize(obj);
    
    Value* cmp = new ICmpInst(ICmpInst::ICMP_ULT, index, size, "",
                              currentBlock);

    BasicBlock* ifTrue =  createBasicBlock("true verifyAndComputePtr");
    BasicBlock* ifFalse = createBasicBlock("false verifyAndComputePtr");

    branch(cmp, ifTrue, ifFalse, currentBlock);
    
    std::vector<Value*>args;
    args.push_back(obj);
    args.push_back(index);
    if (currentExceptionBlock != endExceptionBlock) {
      llvm::InvokeInst::Create(JavaJIT::indexOutOfBoundsExceptionLLVM,
                               unifiedUnreachable,
                               currentExceptionBlock, args.begin(),
                               args.end(), "", ifFalse);
    } else {
      llvm::CallInst::Create(JavaJIT::indexOutOfBoundsExceptionLLVM,
                             args.begin(), args.end(), "", ifFalse);
      new UnreachableInst(ifFalse);
    }
  
    currentBlock = ifTrue;
  }
  
  Constant* zero = mvm::jit::constantZero;
  Value* val = new BitCastInst(obj, arrayType, "", currentBlock);
  
  std::vector<Value*> indexes; //[3];
  indexes.push_back(zero);
  indexes.push_back(JavaArray::elementsOffset());
  indexes.push_back(index);
  Value* ptr = llvm::GetElementPtrInst::Create(val, indexes.begin(),
                                               indexes.end(), 
                                               "", currentBlock);

  return ptr;

}

void JavaJIT::setCurrentBlock(BasicBlock* newBlock) {

  std::vector< std::pair<Value*, const AssessorDesc*> > newStack;
  uint32 index = 0;
  for (BasicBlock::iterator i = newBlock->begin(), e = newBlock->end(); i != e;
       ++i, ++index) {
    // case 2 happens with handlers
    if (!(isa<PHINode>(i)) || i->getType() == mvm::jit::ptrType) {
      break;
    } else {
      const llvm::Type* type = i->getType();
      if (type == Type::Int32Ty || type == Type::Int16Ty || 
          type == Type::Int8Ty) {
        newStack.push_back(std::make_pair(i, AssessorDesc::dInt));
      } else {
        newStack.push_back(std::make_pair(i, stack[index].second));
      }
    }
  }
  
  stack = newStack;
  currentBlock = newBlock;
}

static void testPHINodes(BasicBlock* dest, BasicBlock* insert, JavaJIT* jit) {
  if(dest->empty()) {
    for (std::vector< std::pair<Value*, const AssessorDesc*> >::iterator i =
              jit->stack.begin(),
            e = jit->stack.end(); i!= e; ++i) {
      Value* cur = i->first;
      const AssessorDesc* func = i->second;
      PHINode* node = 0;
      if (func == AssessorDesc::dChar || func == AssessorDesc::dBool) {
        node = llvm::PHINode::Create(Type::Int32Ty, "", dest);
        cur = new ZExtInst(cur, Type::Int32Ty, "", jit->currentBlock);
      } else if (func == AssessorDesc::dByte || func == AssessorDesc::dShort) {
        node = llvm::PHINode::Create(Type::Int32Ty, "", dest);
        cur = new SExtInst(cur, Type::Int32Ty, "", jit->currentBlock);
      } else {
        node = llvm::PHINode::Create(cur->getType(), "", dest);
      }
      node->addIncoming(cur, insert);
    }
  } else {
    std::vector< std::pair<Value*, const AssessorDesc*> >::iterator stackit = 
      jit->stack.begin();
    for (BasicBlock::iterator i = dest->begin(), e = dest->end(); i != e;
         ++i) {
      if (!(isa<PHINode>(i))) {
        break;
      } else {
        Instruction* ins = i;
        Value* cur = stackit->first;
        const AssessorDesc* func = stackit->second;
        
        if (func == AssessorDesc::dChar || func == AssessorDesc::dBool) {
          cur = new ZExtInst(cur, Type::Int32Ty, "", jit->currentBlock);
        } else if (func == AssessorDesc::dByte || func == AssessorDesc::dShort) {
          cur = new SExtInst(cur, Type::Int32Ty, "", jit->currentBlock);
        }
        
        ((PHINode*)ins)->addIncoming(cur, insert);
        ++stackit;
      }
    }
  }
}

void JavaJIT::branch(llvm::BasicBlock* dest, llvm::BasicBlock* insert) {
  testPHINodes(dest, insert, this);
  llvm::BranchInst::Create(dest, insert);
}

void JavaJIT::branch(llvm::Value* test, llvm::BasicBlock* ifTrue,
                     llvm::BasicBlock* ifFalse, llvm::BasicBlock* insert) {  
  testPHINodes(ifTrue, insert, this);
  testPHINodes(ifFalse, insert, this);
  llvm::BranchInst::Create(ifTrue, ifFalse, test, insert);
}

void JavaJIT::makeArgs(FunctionType::param_iterator it,
                       uint32 index, std::vector<Value*>& Args, uint32 nb) {
#ifdef MULTIPLE_VM
  nb++;
#endif
  Args.reserve(nb + 2);
  Value* args[nb];
#ifdef MULTIPLE_VM
  args[nb - 1] = isolateLocal;
  sint32 start = nb - 2;
  it--;
#else
  sint32 start = nb - 1;
#endif
  for (sint32 i = start; i >= 0; --i) {
    it--;
    if (it->get() == Type::Int64Ty || it->get() == Type::DoubleTy) {
      pop();
    }
    const AssessorDesc* func = topFunc();
    Value* tmp = pop();
    
    const Type* type = it->get();
    if (tmp->getType() != type) { // int8 or int16
      if (type == Type::Int32Ty) {
        if (func == AssessorDesc::dChar) {
          tmp = new ZExtInst(tmp, type, "", currentBlock);
        } else {
          tmp = new SExtInst(tmp, type, "", currentBlock);
        }
      } else {
        tmp = new TruncInst(tmp, type, "", currentBlock);
      }
    }
    args[i] = tmp;

  }

  for (uint32 i = 0; i < nb; ++i) {
    Args.push_back(args[i]);
  }
  
}

Instruction* JavaJIT::lowerMathOps(const UTF8* name, 
                                   std::vector<Value*>& args) {
  if (name == Jnjvm::abs) {
    const Type* Ty = args[0]->getType();
    if (Ty == Type::Int32Ty) {
      Constant* const_int32_9 = mvm::jit::constantZero;
      ConstantInt* const_int32_10 = mvm::jit::constantMinusOne;
      BinaryOperator* int32_tmpneg = 
        BinaryOperator::create(Instruction::Sub, const_int32_9, args[0],
                               "tmpneg", currentBlock);
      ICmpInst* int1_abscond = 
        new ICmpInst(ICmpInst::ICMP_SGT, args[0], const_int32_10, "abscond", 
                     currentBlock);
      return llvm::SelectInst::Create(int1_abscond, args[0], int32_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::Int64Ty) {
      Constant* const_int64_9 = mvm::jit::constantLongZero;
      ConstantInt* const_int64_10 = mvm::jit::constantLongMinusOne;
      
      BinaryOperator* int64_tmpneg = 
        BinaryOperator::create(Instruction::Sub, const_int64_9, args[0],
                               "tmpneg", currentBlock);

      ICmpInst* int1_abscond = new ICmpInst(ICmpInst::ICMP_SGT, args[0],
                                            const_int64_10, "abscond",
                                            currentBlock);
      
      return llvm::SelectInst::Create(int1_abscond, args[0], int64_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::FloatTy) {
      return llvm::CallInst::Create(mvm::jit::func_llvm_fabs_f32, args[0],
                                    "tmp1", currentBlock);
    } else if (Ty == Type::DoubleTy) {
      return llvm::CallInst::Create(mvm::jit::func_llvm_fabs_f64, args[0],
                                    "tmp1", currentBlock);
    }
  } else if (name == Jnjvm::sqrt) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_sqrt_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::sin) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_sin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::cos) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_cos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::tan) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_tan_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::asin) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_asin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::acos) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_acos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::atan) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_atan_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::atan2) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_atan2_f64, 
                                  args.begin(), args.end(), "tmp1",
                                  currentBlock);
  } else if (name == Jnjvm::exp) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_exp_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::log) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_log_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::pow) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_pow_f64, args.begin(),
                                  args.end(), "tmp1", currentBlock);
  } else if (name == Jnjvm::ceil) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_ceil_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name == Jnjvm::floor) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_floor_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::rint) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_rint_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::cbrt) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_cbrt_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name == Jnjvm::cosh) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_cosh_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name == Jnjvm::expm1) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_expm1_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::hypot) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_hypot_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::log10) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_log10_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::log1p) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_log1p_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::sinh) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_sinh_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name == Jnjvm::tanh) {
    return llvm::CallInst::Create(mvm::jit::func_llvm_tanh_f64, args[0],
                                  "tmp1", currentBlock);
  }
  
  return 0;

}


Instruction* JavaJIT::invokeInline(JavaMethod* meth, 
                                   std::vector<Value*>& args) {
  JavaJIT jit;
  jit.compilingClass = meth->classDef; 
  jit.compilingMethod = meth;
  jit.unifiedUnreachable = unifiedUnreachable;
  jit.inlineMethods = inlineMethods;
  jit.inlineMethods[meth] = true;
  Instruction* ret = jit.inlineCompile(llvmFunction, currentBlock, 
                                       currentExceptionBlock, args);
  inlineMethods[meth] = false;
  return ret;
}

void JavaJIT::invokeSpecial(uint16 index) {
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  JavaMethod* meth = 0;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* cl = 0;
  ctpInfo->nameOfStaticOrSpecialMethod(index, cl, name, signature);
  llvm::Instruction* val = 0;
    
  std::vector<Value*> args; 
  FunctionType::param_iterator it  = signature->virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  JITVerifyNull(args[0]); 

  if (cl == Jnjvm::mathName) {
    val = lowerMathOps(name, args);
  }


  if (!val) {
    Function* func = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_VIRTUAL,
                                                          signature, meth);
#if 0//def SERVICE_VM
    bool serviceCall = false;
    if (meth && meth->classDef->classLoader != compilingClass->classLoader &&
        meth->classDef->isolate != Jnjvm::bootstrapVM){
      JavaObject* loader = meth->classDef->classLoader;
      ServiceDomain* calling = ServiceDomain::getDomainFromLoader(loader);
      serviceCall = true;
      std::vector<Value*> Args;
      Args.push_back(isolateLocal);
      Args.push_back(new LoadInst(calling->llvmDelegatee(), "", currentBlock));
      CallInst::Create(ServiceDomain::serviceCallStartLLVM, Args.begin(),
                       Args.end(), "", currentBlock);
    }
#endif

    if (meth && meth->canBeInlined && meth != compilingMethod && 
        inlineMethods[meth] == 0) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }

#if 0//def SERVICE_VM
    if (serviceCall) {  
      JavaObject* loader = meth->classDef->classLoader;
      ServiceDomain* calling = ServiceDomain::getDomainFromLoader(loader);
      serviceCall = true;
      std::vector<Value*> Args;
      Args.push_back(isolateLocal);
      Args.push_back(new LoadInst(calling->llvmDelegatee(), "", currentBlock));
      CallInst::Create(ServiceDomain::serviceCallStopLLVM, Args.begin(),
                       Args.end(), "", currentBlock);
    }
#endif
  }
  
  const llvm::Type* retType = signature->virtualType->getReturnType();
  if (retType != Type::VoidTy) {
    push(val, signature->ret->funcs);
    if (retType == Type::DoubleTy || retType == Type::Int64Ty) {
      push(mvm::jit::constantZero, AssessorDesc::dInt);
    }
  }

}

void JavaJIT::invokeStatic(uint16 index) {
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  JavaMethod* meth = 0;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* cl = 0;
  ctpInfo->nameOfStaticOrSpecialMethod(index, cl, name, signature);
  llvm::Instruction* val = 0;
  
  std::vector<Value*> args; // size = [signature->nbIn + 2]; 
  FunctionType::param_iterator it  = signature->staticType->param_end();
  makeArgs(it, index, args, signature->nbIn);
  ctpInfo->markAsStaticCall(index);

  if (cl == Jnjvm::mathName) {
    val = lowerMathOps(name, args);
  }

  if (!val) {
    Function* func = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_STATIC,
                                                          signature, meth);
    
#if 0//def SERVICE_VM
    bool serviceCall = false;
    if (meth && meth->classDef->classLoader != compilingClass->classLoader &&
        meth->classDef->isolate != Jnjvm::bootstrapVM){
      JavaObject* loader = meth->classDef->classLoader;
      ServiceDomain* calling = ServiceDomain::getDomainFromLoader(loader);
      serviceCall = true;
      std::vector<Value*> Args;
      Args.push_back(isolateLocal);
      Args.push_back(new LoadInst(calling->llvmDelegatee(), "", currentBlock));
      CallInst::Create(ServiceDomain::serviceCallStartLLVM, Args.begin(),
                       Args.end(), "", currentBlock);
    }
#endif

    if (meth && meth->canBeInlined && meth != compilingMethod && 
        inlineMethods[meth] == 0) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }

#if 0//def SERVICE_VM
    if (serviceCall) {  
      JavaObject* loader = meth->classDef->classLoader;
      ServiceDomain* calling = ServiceDomain::getDomainFromLoader(loader);
      std::vector<Value*> Args;
      Args.push_back(isolateLocal);
      Args.push_back(new LoadInst(calling->llvmDelegatee(), "", currentBlock));
      CallInst::Create(ServiceDomain::serviceCallStopLLVM, Args.begin(), Args.end(), "",
                       currentBlock);
    }
#endif
  }

  const llvm::Type* retType = signature->staticType->getReturnType();
  if (retType != Type::VoidTy) {
    push(val, signature->ret->funcs);
    if (retType == Type::DoubleTy || retType == Type::Int64Ty) {
      push(mvm::jit::constantZero, AssessorDesc::dInt);
    }
  }
}
    
Value* JavaJIT::getResolvedClass(uint16 index, bool clinit) {
    const Type* PtrTy = mvm::jit::ptrType;
    compilingClass->isolate->protectModule->lock();
    GlobalVariable * gv =
      new GlobalVariable(PtrTy, false, 
                         GlobalValue::ExternalLinkage,
                         mvm::jit::constantPtrNull, "",
                         compilingClass->isolate->module);

    compilingClass->isolate->protectModule->unlock();
    
    Value* arg1 = new LoadInst(gv, "", false, currentBlock);
    Value* test = new ICmpInst(ICmpInst::ICMP_EQ, arg1, 
                               mvm::jit::constantPtrNull, "", currentBlock);
    
    BasicBlock* trueCl = createBasicBlock("Cl OK");
    BasicBlock* falseCl = createBasicBlock("Cl Not OK");
    PHINode* node = llvm::PHINode::Create(PtrTy, "", trueCl);
    node->addIncoming(arg1, currentBlock);
    llvm::BranchInst::Create(falseCl, trueCl, test, currentBlock);

    currentBlock = falseCl;

    std::vector<Value*> Args;
    Value* v = 
      new LoadInst(compilingClass->llvmVar(compilingClass->isolate->module), 
                   "", currentBlock);
    Args.push_back(v);
    mvm::jit::protectConstants();
    ConstantInt* CI = ConstantInt::get(Type::Int32Ty, index);
    mvm::jit::unprotectConstants();
    Args.push_back(CI);
    Args.push_back(gv);
    if (clinit) {
      Args.push_back(mvm::jit::constantOne);
    } else {
      Args.push_back(mvm::jit::constantZero);
    }
    Value* res = invoke(newLookupLLVM, Args, "", currentBlock);
    node->addIncoming(res, currentBlock);

    llvm::BranchInst::Create(trueCl, currentBlock);
    currentBlock = trueCl;
#ifdef MULTIPLE_VM
    invoke(initialisationCheckLLVM, node, "", currentBlock);
#endif
    
    return node;
}

void JavaJIT::invokeNew(uint16 index) {
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  ctpInfo->checkInfoOfClass(index);
  
  Class* cl = (Class*)(ctpInfo->getMethodClassIfLoaded(index));
#ifdef SERVICE_VM
  if (cl && cl->classLoader != compilingClass->classLoader && 
      cl->isolate != Jnjvm::bootstrapVM) {
    ServiceDomain* vm = (ServiceDomain*)JavaThread::get()->isolate;
    assert(vm->getVirtualTable() == ServiceDomain::VT && "vm not a service?");
    vm->serviceError("The service called new on another service");
  }
#endif
  Value* val = 0;
  if (!cl || !(cl->isResolved())
#ifndef MULTIPLE_VM
      || !cl->isReady()
#endif
     ) {
    Value* node = getResolvedClass(index, true);
#ifndef MULTIPLE_VM
    val = invoke(doNewUnknownLLVM, node, "", currentBlock);
#else
    val = invoke(doNewUnknownLLVM, node, isolateLocal, "", currentBlock);
#endif
  } else {
    Value* load = new LoadInst(cl->llvmVar(compilingClass->isolate->module),
                               "", currentBlock);
#ifdef MULTIPLE_VM
    Module* M = compilingClass->isolate->module;
    Value* arg = new LoadInst(cl->llvmVar(M), "", currentBlock);
    invoke(initialisationCheckLLVM, arg, "", currentBlock);
    val = invoke(doNewLLVM, load, isolateLocal, "", currentBlock);
#else
    val = invoke(doNewLLVM, load, "", currentBlock);
#endif
    // give the real type info, escape analysis uses it
    new BitCastInst(val, cl->virtualType, "", currentBlock);
  }
  
  push(val, AssessorDesc::dRef);

}

Value* JavaJIT::arraySize(Value* val) {
  return llvm::CallInst::Create(arrayLengthLLVM, val, "", currentBlock);
  /*
  Value* array = new BitCastInst(val, JavaArray::llvmType, "", currentBlock);
  std::vector<Value*> args; //size=  2
  args.push_back(mvm::jit::constantZero);
  args.push_back(JavaArray::sizeOffset());
  Value* ptr = llvm::GetElementPtrInst::Create(array, args.begin(), args.end(),
                                     "", currentBlock);
  return new LoadInst(ptr, "", currentBlock);*/
}

static llvm::Value* fieldGetter(JavaJIT* jit, const Type* type, Value* object,
                                Value* offset) {
  llvm::Value* objectConvert = new BitCastInst(object, type, "",
                                               jit->currentBlock);

  Constant* zero = mvm::jit::constantZero;
  std::vector<Value*> args; // size = 2
  args.push_back(zero);
  args.push_back(offset);
  llvm::Value* ptr = llvm::GetElementPtrInst::Create(objectConvert,
                                                     args.begin(),
                                                     args.end(), "",
                                                     jit->currentBlock);
  return ptr;  
}

Value* JavaJIT::ldResolved(uint16 index, bool stat, Value* object, 
                           const Type* fieldType, const Type* fieldTypePtr) {
  JavaCtpInfo* info = compilingClass->ctpInfo;
  
  JavaField* field = info->lookupField(index, stat);
  if (field && field->classDef->isResolved()
#ifndef MULTIPLE_VM
      && field->classDef->isReady()
#endif
     ) {
    if (stat) object = field->classDef->staticVar(this);
    const Type* type = stat ? field->classDef->staticType :
                              field->classDef->virtualType;
    return fieldGetter(this, type, object, field->offset);
  } else {
    const Type* Pty = mvm::jit::arrayPtrType;
    compilingClass->isolate->protectModule->lock();
    GlobalVariable* gvStaticInstance = 
      new GlobalVariable(mvm::jit::ptrType, false, 
                         GlobalValue::ExternalLinkage,
                         mvm::jit::constantPtrNull, 
                         "", compilingClass->isolate->module);
    
    
    Constant* zero = mvm::jit::constantZero;
    GlobalVariable* gv = 
      new GlobalVariable(Type::Int32Ty, false, GlobalValue::ExternalLinkage,
                         zero, "", compilingClass->isolate->module);
    compilingClass->isolate->protectModule->unlock();
    
    // set is volatile
    Value* val = new LoadInst(gv, "", true, currentBlock);
    Value * cmp = new ICmpInst(ICmpInst::ICMP_NE, val, zero, "", currentBlock);
    BasicBlock* ifTrue  = createBasicBlock("true ldResolved");
    BasicBlock* ifFalse  = createBasicBlock("false ldResolved");
    BasicBlock* endBlock  = createBasicBlock("end ldResolved");
    PHINode * node = llvm::PHINode::Create(mvm::jit::ptrType, "", endBlock);
    llvm::BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
  
    // ---------- In case we already resolved something --------------------- //
    currentBlock = ifTrue;
    Value* resPtr = 0;
    if (object) {
      Value* ptr = new BitCastInst(object, Pty, "", currentBlock);
      std::vector<Value*> gepArgs; // size = 1
      gepArgs.push_back(zero);
      gepArgs.push_back(val);
      resPtr = llvm::GetElementPtrInst::Create(ptr, gepArgs.begin(), gepArgs.end(),
                                     "", currentBlock);
    
    } else {
      resPtr = new LoadInst(gvStaticInstance, "", currentBlock);
    }
    
    node->addIncoming(resPtr, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);

    // ---------- In case we have to resolve -------------------------------- //
    currentBlock = ifFalse;
    std::vector<Value*> args;
    if (object) {
      args.push_back(object);
    } else {
      args.push_back(constantJavaObjectNull);
    }
    Module* M = compilingClass->isolate->module;
    args.push_back(new LoadInst(compilingClass->llvmVar(M), "", currentBlock));
    mvm::jit::protectConstants();//->lock();
    Constant* CI = ConstantInt::get(Type::Int32Ty, index);
    args.push_back(CI);
    mvm::jit::unprotectConstants();//->unlock();
    args.push_back(stat ? mvm::jit::constantOne : mvm::jit::constantZero);
    args.push_back(gvStaticInstance);
    args.push_back(gv);
    Value* tmp = invoke(JavaJIT::fieldLookupLLVM, args, "", currentBlock);
    node->addIncoming(tmp, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);
    
    currentBlock = endBlock;;
    return new BitCastInst(node, fieldTypePtr, "", currentBlock);
  }
}

extern void convertValue(Value*& val, const Type* t1, BasicBlock* currentBlock,
                         bool usign);
 

void JavaJIT::setStaticField(uint16 index) {
  const AssessorDesc* ass = topFunc();
  Value* val = pop(); 
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  const Type* type = sign->funcs->llvmType;
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    val = pop();
  }
  Value* ptr = ldResolved(index, true, 0, type, sign->funcs->llvmTypePtr);
  
  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, 
                 ass == AssessorDesc::dChar || ass == AssessorDesc::dBool);
  }
  
  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getStaticField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  Value* ptr = ldResolved(index, true, 0, sign->funcs->llvmType, 
                          sign->funcs->llvmTypePtr);
  push(new LoadInst(ptr, "", currentBlock), sign->funcs);
  const Type* type = sign->funcs->llvmType;
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    push(mvm::jit::constantZero, AssessorDesc::dInt);
  }
}

void JavaJIT::setVirtualField(uint16 index) {
  const AssessorDesc* ass = topFunc();
  Value* val = pop();
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  const Type* type = sign->funcs->llvmType;
  
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    val = pop();
  }
  
  Value* object = pop();
  JITVerifyNull(object);
  Value* ptr = ldResolved(index, false, object, type, 
                          sign->funcs->llvmTypePtr);

  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, 
                 ass == AssessorDesc::dChar || ass == AssessorDesc::dBool);
  }

  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getVirtualField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  Value* obj = pop();
  JITVerifyNull(obj);
  Value* ptr = ldResolved(index, false, obj, sign->funcs->llvmType, 
                          sign->funcs->llvmTypePtr);
  push(new LoadInst(ptr, "", currentBlock), sign->funcs);
  const Type* type = sign->funcs->llvmType;
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    push(mvm::jit::constantZero, AssessorDesc::dInt);
  }
}


Instruction* JavaJIT::invoke(Value *F, std::vector<llvm::Value*>& args,
                       const char* Name,
                       BasicBlock *InsertAtEnd) {
  
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    args.begin(), 
                                    args.end(), Name, InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, const char* Name,
                       BasicBlock *InsertAtEnd) {

  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    std::vector<Value*> arg;
    arg.push_back(arg1);
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    arg.begin(), arg.end(), Name, InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, arg1, Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, Value* arg2,
                       const char* Name, BasicBlock *InsertAtEnd) {

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    args.begin(), args.end(), Name,
                                    InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, const char* Name,
                       BasicBlock *InsertAtEnd) {
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    std::vector<Value*> args;
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    args.begin(), args.end(), Name,
                                    InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, Name, InsertAtEnd);
  }
}


