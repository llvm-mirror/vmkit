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
  LLVMSignatureInfo* LSI = module->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  
  JITVerifyNull(args[0]); 

  Value* VT = CallInst::Create(JnjvmModule::GetVTFunction, args[0], "",
                               currentBlock);
  std::vector<Value*> indexes2; //[3];
  if (meth) {
    LLVMMethodInfo* LMI = module->getMethodInfo(meth);
    indexes2.push_back(LMI->getOffset());
  } else {
    GlobalVariable* gv = 
      new GlobalVariable(Type::Int32Ty, false, GlobalValue::ExternalLinkage,
                         zero, "", module);
    
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
    LLVMClassInfo* LCI = 
      (LLVMClassInfo*)module->getClassInfo(compilingClass);
    Args.push_back(LCI->getVar(this));
    Constant* CI = ConstantInt::get(Type::Int32Ty, index);
    Args.push_back(CI);
    Args.push_back(gv);
    val = invoke(JnjvmModule::VirtualLookupFunction, Args, "", currentBlock);
    node->addIncoming(val, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);
    
    currentBlock = endBlock;
    indexes2.push_back(node);
  }
  
  Value* FuncPtr = GetElementPtrInst::Create(VT, indexes2.begin(),
                                             indexes2.end(), "",
                                             currentBlock);
    
  Value* Func = new LoadInst(FuncPtr, "", currentBlock);
  Func = new BitCastInst(Func, LSI->getVirtualPtrType(), "", currentBlock);

  Value* val = invoke(Func, args, "", currentBlock);
  
  const llvm::Type* retType = virtualType->getReturnType();
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

llvm::Function* JavaJIT::nativeCompile(void* natPtr) {
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "native compile %s\n",
              compilingMethod->printString());
  
  bool stat = isStatic(compilingMethod->access);

  const FunctionType *funcType = llvmFunction->getFunctionType();
  
  bool jnjvm = false;
  natPtr = natPtr ? natPtr :
              NativeUtil::nativeLookup(compilingClass, compilingMethod, jnjvm);
  
  
  
  Function* func = llvmFunction;
  if (jnjvm) {
    mvm::jit::executionEngine->addGlobalMapping(func, natPtr);
    return llvmFunction;
  }
  
  currentExceptionBlock = endExceptionBlock = 0;
  currentBlock = createBasicBlock("start");
  BasicBlock* executeBlock = createBasicBlock("execute");
  endBlock = createBasicBlock("end block");

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
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
    LLVMServiceInfo* LSI = module->getServiceInfo(vm);
    isolateLocal = LSI->getDelegatee(this);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, lastArg, 
                              isolateLocal, "", currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(lastArg);
    Args.push_back(isolateLocal);
    CallInst::Create(JnjvmModule::ServiceCallStartFunction, Args.begin(),
                     Args.end(), "", currentBlock);
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
  }
#endif
#endif

  if (funcType->getReturnType() != Type::VoidTy)
    endNode = llvm::PHINode::Create(funcType->getReturnType(), "", endBlock);
  
  Value* buf = llvm::CallInst::Create(JnjvmModule::GetSJLJBufferFunction,
                                      "", currentBlock);
  Value* test = llvm::CallInst::Create(mvm::jit::setjmpLLVM, buf, "",
                                       currentBlock);
  test = new ICmpInst(ICmpInst::ICMP_EQ, test, mvm::jit::constantZero, "",
                      currentBlock);
  llvm::BranchInst::Create(executeBlock, endBlock, test, currentBlock);

  if (compilingMethod->signature->ret->funcs != AssessorDesc::dVoid) {
    uint8 id = compilingMethod->signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    Constant* C = LAI.llvmNullConstant;
    endNode->addIncoming(C, currentBlock);
  }
  
  currentBlock = executeBlock;
  if (isSynchro(compilingMethod->access))
    beginSynchronize();
  
  
  uint32 nargs = func->arg_size() + 1 + (stat ? 1 : 0); 
  std::vector<Value*> nativeArgs;
  
  int64_t jniEnv = (int64_t)&(compilingClass->isolate->jniEnv);
  nativeArgs.push_back(
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, jniEnv), 
                              mvm::jit::ptrType));

  uint32 index = 0;
  if (stat) {
    LLVMClassInfo* LCI = 
      (LLVMClassInfo*)module->getClassInfo(compilingClass);
    nativeArgs.push_back(LCI->getDelegatee(this));
    index = 2;
  } else {
    index = 1;
  }
  for (Function::arg_iterator i = func->arg_begin(); 
       index < nargs; ++i, ++index) {
     
    nativeArgs.push_back(i);
  }
  
  
  LLVMSignatureInfo* LSI = 
    module->getSignatureInfo(compilingMethod->signature);
  const llvm::Type* valPtrType = LSI->getNativePtrType();
  Value* valPtr = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, (uint64)natPtr),
                              valPtrType);

  Value* result = llvm::CallInst::Create(valPtr, nativeArgs.begin(),
                                         nativeArgs.end(), "", currentBlock);

  if (funcType->getReturnType() != Type::VoidTy)
    endNode->addIncoming(result, currentBlock);
  llvm::BranchInst::Create(endBlock, currentBlock);

  currentBlock = endBlock; 
  if (isSynchro(compilingMethod->access))
    endSynchronize();
  
  llvm::CallInst::Create(JnjvmModule::JniProceedPendingExceptionFunction, "",
                         currentBlock);
  
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
    LLVMClassInfo* LCI = 
      (LLVMClassInfo*)module->getClassInfo(compilingClass);
    Value* arg = LCI->getStaticVar(this);
    argsSync.push_back(arg);
  }
#ifdef SERVICE_VM
  if (ServiceDomain::isLockableDomain(compilingClass->isolate))
    llvm::CallInst::Create(JnjvmModule::AquireObjectInSharedDomainFunction,
                           argsSync.begin(), argsSync.end(), "", currentBlock);
  else
#endif
  llvm::CallInst::Create(JnjvmModule::AquireObjectFunction, argsSync.begin(),
                         argsSync.end(), "", currentBlock);
}

void JavaJIT::endSynchronize() {
  std::vector<Value*> argsSync;
  if (isVirtual(compilingMethod->access)) {
    argsSync.push_back(llvmFunction->arg_begin());
  } else {
    LLVMClassInfo* LCI = 
      (LLVMClassInfo*)module->getClassInfo(compilingClass);
    Value* arg = LCI->getStaticVar(this);
    argsSync.push_back(arg);
  }
#ifdef SERVICE_VM
  if (ServiceDomain::isLockableDomain(compilingClass->isolate))
    llvm::CallInst::Create(JnjvmModule::ReleaseObjectInSharedDomainFunction,
                           argsSync.begin(), argsSync.end(), "", currentBlock);
  else
#endif
  llvm::CallInst::Create(JnjvmModule::ReleaseObjectFunction, argsSync.begin(),
                         argsSync.end(), "", currentBlock);    
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
  
  const FunctionType *funcType = llvmFunction->getFunctionType();
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
    objectLocals.push_back(new AllocaInst(JnjvmModule::JavaObjectType, "",
                                          currentBlock));
  }
  
  uint32 index = 0;
  uint32 count = 0;
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  uint32 max = args.size() - 1;
#else
  uint32 max = args.size();
#endif
  std::vector<Typedef*>::iterator type = 
    compilingMethod->signature->args.begin();
  std::vector<Value*>::iterator i = args.begin();

  if (isVirtual(compilingMethod->access)) {
    new StoreInst(*i, objectLocals[0], false, currentBlock);
    ++i;
    ++index;
    ++count;
  }
  
  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const AssessorDesc* cur = (*type)->funcs;

    if (cur == AssessorDesc::dLong){
      new StoreInst(*i, longLocals[index], false, currentBlock);
      ++index;
    } else if (AssessorDesc::dBool || cur == AssessorDesc::dChar) {
      new StoreInst(new ZExtInst(*i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (AssessorDesc::dByte || cur == AssessorDesc::dShort) {
      new StoreInst(new SExtInst(*i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (cur == AssessorDesc::dInt) {
      new StoreInst(*i, intLocals[index], false, currentBlock);
    } else if (cur == AssessorDesc::dDouble) {
      new StoreInst(*i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (cur == AssessorDesc::dFloat) {
      new StoreInst(*i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(*i, objectLocals[index], false, currentBlock);
    }
  }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
#if !defined(SERVICE_VM)
  isolateLocal = args[args.size() - 1];
#else
  if (compilingClass->isolate == Jnjvm::bootstrapVM) {
    isolateLocal = args[args.size() - 1];
  } else {
    JavaObject* loader = compilingClass->classLoader;
    ServiceDomain* vm = ServiceDomain::getDomainFromLoader(loader);
    LLVMServiceInfo* LSI = module->getServiceInfo(vm);
    isolateLocal = LSI->getDelegatee(this);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, args[args.size() - 1], 
                              isolateLocal, "", currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(args[args.size()-  1]);
    Args.push_back(isolateLocal);
    CallInst::Create(JnjvmModule::ServiceCallStartFunction, Args.begin(),
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
    CallInst::Create(JnjvmModule::ServiceCallStopFunction, Args.begin(),
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

  const FunctionType *funcType = llvmFunction->getFunctionType();
  returnType = funcType->getReturnType();
  
  Function* func = llvmFunction;

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
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
    llvm::CallInst::Create(printMethodStartLLVM, args.begin(), args.end(), "",
                           currentBlock);
    }
#endif

  

  for (int i = 0; i < maxLocals; i++) {
    intLocals.push_back(new AllocaInst(Type::Int32Ty, "", currentBlock));
    doubleLocals.push_back(new AllocaInst(Type::DoubleTy, "", currentBlock));
    longLocals.push_back(new AllocaInst(Type::Int64Ty, "", currentBlock));
    floatLocals.push_back(new AllocaInst(Type::FloatTy, "", currentBlock));
    objectLocals.push_back(new AllocaInst(JnjvmModule::JavaObjectType, "",
                                          currentBlock));
  }
  
  uint32 index = 0;
  uint32 count = 0;
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  uint32 max = func->arg_size() - 1;
#else
  uint32 max = func->arg_size();
#endif
  Function::arg_iterator i = func->arg_begin(); 
  std::vector<Typedef*>::iterator type = 
    compilingMethod->signature->args.begin();

  if (isVirtual(compilingMethod->access)) {
    new StoreInst(i, objectLocals[0], false, currentBlock);
    ++i;
    ++index;
    ++count;
  }

  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const AssessorDesc* cur = (*type)->funcs;
    if (cur == AssessorDesc::dLong){
      new StoreInst(i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur == AssessorDesc::dBool || cur == AssessorDesc::dChar) {
      new StoreInst(new ZExtInst(i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (cur == AssessorDesc::dByte || cur == AssessorDesc::dShort) {
      new StoreInst(new SExtInst(i, Type::Int32Ty, "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (cur == AssessorDesc::dInt) {
      new StoreInst(i, intLocals[index], false, currentBlock);
    } else if (cur == AssessorDesc::dDouble) {
      new StoreInst(i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (cur == AssessorDesc::dFloat) {
      new StoreInst(i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(i, objectLocals[index], false, currentBlock);
    }
  }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
#if !defined(SERVICE_VM)
  isolateLocal = i;
#else
  if (compilingClass->isolate == Jnjvm::bootstrapVM) {
    isolateLocal = i;
  } else {
    JavaObject* loader = compilingClass->classLoader;
    ServiceDomain* vm = ServiceDomain::getDomainFromLoader(loader);
    LLVMServiceInfo* LSI = module->getServiceInfo(vm);
    isolateLocal = LSI->getDelegatee(this);
    Value* cmp = new ICmpInst(ICmpInst::ICMP_NE, i, isolateLocal, "",
                              currentBlock);
    BasicBlock* ifTrue = createBasicBlock("true service call");
    BasicBlock* endBlock = createBasicBlock("end check service call");
    BranchInst::Create(ifTrue, endBlock, cmp, currentBlock);
    currentBlock = ifTrue;
    std::vector<Value*> Args;
    Args.push_back(i);
    Args.push_back(isolateLocal);
    CallInst::Create(JnjvmModule::ServiceCallStartFunction, Args.begin(),
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
    args.push_back(ConstantInt::get(Type::Int32Ty, (int64_t)compilingMethod));
    llvm::CallInst::Create(JnjvmModule::PrintMethodEndFunction, args.begin(),
                           args.end(), "", currentBlock);
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
    CallInst::Create(JnjvmModule::ServiceCallStopFunction, Args.begin(),
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
    CallInst* ptr_eh_ptr = CallInst::Create(JnjvmModule::GetExceptionFunction,
                                            "eh_ptr", endExceptionBlock);
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
  
  /*
  if (compilingMethod->name == 
      compilingClass->isolate->asciizConstructUTF8("main")) {
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
  }
  */

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
    supplLocal = new AllocaInst(JnjvmModule::JavaObjectType, "exceptionVar",
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
      LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(compilingClass);
      Value* arg = LCI->getStaticVar(this);
      argsSync.push_back(arg);
    }
    llvm::CallInst::Create(JnjvmModule::ReleaseObjectFunction, argsSync.begin(), argsSync.end(),
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
  
  // We don't need the lock here, and Java requires to load the classes in the
  // try clause, which may require compilation. Therefore we release the lock
  // and aquire it after the exception table is read.
  mvm::jit::executionEngine->lock.release();
  for (uint16 i = 0; i < nbe - sync; ++i) {
    Exception* ex = new Exception();
    ex->startpc   = reader->readU2();
    ex->endpc     = reader->readU2();
    ex->handlerpc = reader->readU2();

    ex->catche = reader->readU2();

    if (ex->catche) {
      JavaObject* exc = 0;
      Class* cl = 0; 
      try {
        cl = (Class*)(ctpInfo->loadClass(ex->catche));
      } catch(...) {
        compilingClass->release();
        exc = JavaThread::getJavaException();
        assert(exc && "no exception?");
        JavaThread::clearException();
      }
      
      if (exc) {
        Jnjvm* vm = JavaThread::get()->isolate;
        vm->errorWithExcp(Jnjvm::NoClassDefFoundError, exc);
      }

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
  mvm::jit::executionEngine->lock.acquire();
  
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
    
    Value* cl = 0;
    currentBlock = cur->realTest;
    assert(cur->catchClass);
    LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(cur->catchClass);
    cl = LCI->getVar(this);
    Value* cmp = llvm::CallInst::Create(JnjvmModule::CompareExceptionFunction, cl, "",
                                        currentBlock);
    llvm::BranchInst::Create(cur->handler, bbNext, cmp, currentBlock);
    if (nodeNext)
      nodeNext->addIncoming(cur->exceptionPHI, currentBlock);
    
    if (cur->handler->empty()) {
      cur->handlerPHI = llvm::PHINode::Create(mvm::jit::ptrType, "",
                                              cur->handler);
      cur->handlerPHI->addIncoming(cur->exceptionPHI, currentBlock);
      Value* exc = llvm::CallInst::Create(JnjvmModule::GetJavaExceptionFunction,
                                          "", cur->handler);
      llvm::CallInst::Create(JnjvmModule::ClearExceptionFunction, "",
                             cur->handler);
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
        gv =
          new GlobalVariable(JnjvmModule::JavaObjectType, false, 
                             GlobalValue::ExternalLinkage,
                             JnjvmModule::JavaObjectNullConstant, "",
                             module);
#else
          val = (void*)utf8;
          gv =
            new GlobalVariable(JnjvmModule::ArrayUInt16Type, false, 
                               GlobalValue::ExternalLinkage,
                               UTF8NullConstant, "",
                               module);
#endif
        
        // TODO: put an initialiser in here
        void* ptr = mvm::jit::executionEngine->getPointerToGlobal(gv);
        GenericValue Val = GenericValue(val);
        llvm::GenericValue * Ptr = (llvm::GenericValue*)ptr;
        mvm::jit::executionEngine->StoreValueToMemory(Val, Ptr,
                                                  JnjvmModule::JavaObjectType);
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
    CallInst* C = llvm::CallInst::Create(JnjvmModule::RuntimeUTF8ToStrFunction,
                                         toPush, "", currentBlock);
    push(C, AssessorDesc::dRef);
#else
    push(toPush, AssessorDesc::dRef);
#endif
  } else if (type == JavaCtpInfo::ConstantLong) {
    push(ConstantInt::get(Type::Int64Ty, ctpInfo->LongAt(index)),
         AssessorDesc::dLong);
  } else if (type == JavaCtpInfo::ConstantDouble) {
    push(ConstantFP::get(Type::DoubleTy, ctpInfo->DoubleAt(index)),
         AssessorDesc::dDouble);
  } else if (type == JavaCtpInfo::ConstantInteger) {
    push(ConstantInt::get(Type::Int32Ty, ctpInfo->IntegerAt(index)),
         AssessorDesc::dInt);
  } else if (type == JavaCtpInfo::ConstantFloat) {
    push(ConstantFP::get(Type::FloatTy, ctpInfo->FloatAt(index)),
         AssessorDesc::dFloat);
  } else if (type == JavaCtpInfo::ConstantClass) {
    if (ctpInfo->ctpRes[index]) {
      CommonClass* cl = (CommonClass*)(ctpInfo->ctpRes[index]);
      LLVMCommonClassInfo* LCI = module->getClassInfo(cl);
      push(LCI->getDelegatee(this), AssessorDesc::dRef);
    } else {
      Value* val = getResolvedClass(index, false);
      Value* res = CallInst::Create(JnjvmModule::GetClassDelegateeFunction, val, "",
                                    currentBlock);
      push(res, AssessorDesc::dRef);
    }
  } else {
    JavaThread::get()->isolate->unknownError("unknown type %d", type);
  }
}

void JavaJIT::JITVerifyNull(Value* obj) { 

  JavaJIT* jit = this;
  Constant* zero = JnjvmModule::JavaObjectNullConstant;
  Value* test = new ICmpInst(ICmpInst::ICMP_EQ, obj, zero, "",
                             jit->currentBlock);

  BasicBlock* exit = jit->createBasicBlock("verifyNullExit");
  BasicBlock* cont = jit->createBasicBlock("verifyNullCont");

  llvm::BranchInst::Create(exit, cont, test, jit->currentBlock);
  std::vector<Value*> args;
  if (currentExceptionBlock != endExceptionBlock) {
    llvm::InvokeInst::Create(JnjvmModule::NullPointerExceptionFunction,
                             unifiedUnreachable,
                             currentExceptionBlock, args.begin(),
                             args.end(), "", exit);
  } else {
    llvm::CallInst::Create(JnjvmModule::NullPointerExceptionFunction,
                           args.begin(), args.end(), "", exit);
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
      llvm::InvokeInst::Create(JnjvmModule::IndexOutOfBoundsExceptionFunction,
                               unifiedUnreachable,
                               currentExceptionBlock, args.begin(),
                               args.end(), "", ifFalse);
    } else {
      llvm::CallInst::Create(JnjvmModule::IndexOutOfBoundsExceptionFunction,
                             args.begin(), args.end(), "", ifFalse);
      new UnreachableInst(ifFalse);
    }
  
    currentBlock = ifTrue;
  }
  
  Constant* zero = mvm::jit::constantZero;
  Value* val = new BitCastInst(obj, arrayType, "", currentBlock);
  
  std::vector<Value*> indexes; //[3];
  indexes.push_back(zero);
  indexes.push_back(JnjvmModule::JavaArrayElementsOffsetConstant);
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
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  nb++;
#endif
  Args.reserve(nb + 2);
  Value* args[nb];
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
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
  jit.module = module;
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
  LLVMSignatureInfo* LSI = module->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  llvm::Instruction* val = 0;
  
  std::vector<Value*> args; 
  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  JITVerifyNull(args[0]); 

  if (cl == Jnjvm::mathName) {
    val = lowerMathOps(name, args);
  }


  if (!val) {
    Function* func = 
      (Function*)ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_VIRTUAL,
                                                      signature, meth);

    if (meth && meth->canBeInlined && meth != compilingMethod && 
        inlineMethods[meth] == 0) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }

  }
  
  const llvm::Type* retType = virtualType->getReturnType();
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
  LLVMSignatureInfo* LSI = module->getSignatureInfo(signature);
  const llvm::FunctionType* staticType = LSI->getStaticType();
  llvm::Instruction* val = 0;
  
  std::vector<Value*> args; // size = [signature->nbIn + 2]; 
  FunctionType::param_iterator it  = staticType->param_end();
  makeArgs(it, index, args, signature->nbIn);
  ctpInfo->markAsStaticCall(index);

  if (cl == Jnjvm::mathName) {
    val = lowerMathOps(name, args);
  }

  if (!val) {
    Function* func = (Function*)
      ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_STATIC,
                                           signature, meth);
    
#ifdef MULTIPLE_VM
    
    uint32 clIndex = ctpInfo->getClassIndexFromMethod(index);
    Class* mycl = (Class*)(ctpInfo->getMethodClassIfLoaded(clIndex));
    if (mycl && mycl->status >= resolved) {
      LLVMCommonClassInfo* LCI = module->getClassInfo(mycl);
      Value* arg = LCI->getVar(this);
      arg = invoke(JnjvmModule::InitialisationCheckFunction, arg, "",
                   currentBlock);
      CallInst::Create(JnjvmModule::ForceInitialisationCheckFunction, arg, "",
                       currentBlock);
    }

#endif

    if (meth && meth->canBeInlined && meth != compilingMethod && 
        inlineMethods[meth] == 0) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }

  }

  const llvm::Type* retType = staticType->getReturnType();
  if (retType != Type::VoidTy) {
    push(val, signature->ret->funcs);
    if (retType == Type::DoubleTy || retType == Type::Int64Ty) {
      push(mvm::jit::constantZero, AssessorDesc::dInt);
    }
  }
}
    
Value* JavaJIT::getResolvedClass(uint16 index, bool clinit) {
    GlobalVariable * gv =
      new GlobalVariable(JnjvmModule::JavaClassType, false, 
                         GlobalValue::ExternalLinkage,
                         JnjvmModule::JavaClassNullConstant, "",
                         module);

    
    Value* arg1 = new LoadInst(gv, "", false, currentBlock);
    Value* test = new ICmpInst(ICmpInst::ICMP_EQ, arg1, 
                               JnjvmModule::JavaClassNullConstant, "",
                               currentBlock);
    
    BasicBlock* trueCl = createBasicBlock("Cl OK");
    BasicBlock* falseCl = createBasicBlock("Cl Not OK");
    PHINode* node = llvm::PHINode::Create(JnjvmModule::JavaClassType, "",
                                          trueCl);
    node->addIncoming(arg1, currentBlock);
    llvm::BranchInst::Create(falseCl, trueCl, test, currentBlock);

    currentBlock = falseCl;

    std::vector<Value*> Args;
    LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(compilingClass);
    Value* v = LCI->getVar(this);
    Args.push_back(v);
    ConstantInt* CI = ConstantInt::get(Type::Int32Ty, index);
    Args.push_back(CI);
    Args.push_back(gv);
    if (clinit) {
      Args.push_back(mvm::jit::constantOne);
    } else {
      Args.push_back(mvm::jit::constantZero);
    }
    Value* res = invoke(JnjvmModule::ClassLookupFunction, Args, "",
                        currentBlock);
    node->addIncoming(res, currentBlock);

    llvm::BranchInst::Create(trueCl, currentBlock);
    currentBlock = trueCl;
#ifdef MULTIPLE_VM
    if (clinit)
      return invoke(JnjvmModule::InitialisationCheckFunction, node, "",
                    currentBlock);
    else
#endif
    return node;
}

void JavaJIT::invokeNew(uint16 index) {
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  ctpInfo->checkInfoOfClass(index);
  
  Class* cl = (Class*)(ctpInfo->getMethodClassIfLoaded(index));
  Value* Size = 0;
  Value* VT = 0;
  Value* Cl = 0;
  if (!cl || !cl->isResolved()) {
    Cl = getResolvedClass(index, true);
    Size = CallInst::Create(JnjvmModule::GetObjectSizeFromClassFunction, Cl,
                            "", currentBlock);
    VT = CallInst::Create(JnjvmModule::GetVTFromClassFunction, Cl, "",
                          currentBlock);
  } else {
    LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(cl);
    Size = LCI->getVirtualSize(this);
    VT = LCI->getVirtualTable(this);
    Cl = LCI->getVar(this);
#ifndef MULTIPLE_VM
    if (!cl->isReady())
#endif
      Cl = invoke(JnjvmModule::InitialisationCheckFunction, Cl, "", currentBlock);
  }
  std::vector<Value*> args;
  args.push_back(Size);
  args.push_back(VT);
#ifdef MULTIPLE_GC
  args.push_back(CallInst::Create(JnjvmModule::GetCollectorFunction,
                                  isolateLocal, "", currentBlock));
#endif
  Value* val = invoke(JnjvmModule::JavaObjectAllocateFunction, args, "",
                      currentBlock);
  
  // Set the class
  
  std::vector<Value*> gep;
  gep.push_back(mvm::jit::constantZero);
  gep.push_back(JnjvmModule::JavaObjectClassOffsetConstant);
  Value* GEP = GetElementPtrInst::Create(val, gep.begin(), gep.end(), "",
                                         currentBlock);
  new StoreInst(Cl, GEP, currentBlock);


  push(val, AssessorDesc::dRef);
}

Value* JavaJIT::arraySize(Value* val) {
  return llvm::CallInst::Create(JnjvmModule::ArrayLengthFunction, val, "",
                                currentBlock);
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
    LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(field->classDef);
    if (stat) {
      object = LCI->getStaticVar(this);
    }
    const Type* type = stat ? LCI->getStaticType() :
                              LCI->getVirtualType();
    LLVMFieldInfo* LFI = module->getFieldInfo(field);
    return fieldGetter(this, type, object, LFI->getOffset());
  } else {
    const Type* Pty = mvm::jit::arrayPtrType;
    GlobalVariable* gvStaticInstance = 
      new GlobalVariable(mvm::jit::ptrType, false, 
                         GlobalValue::ExternalLinkage,
                         mvm::jit::constantPtrNull, 
                         "", module);
    
    
    Constant* zero = mvm::jit::constantZero;
    GlobalVariable* gv = 
      new GlobalVariable(Type::Int32Ty, false, GlobalValue::ExternalLinkage,
                         zero, "", module);
    
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
      args.push_back(JnjvmModule::JavaObjectNullConstant);
    }
    LLVMClassInfo* LCI = (LLVMClassInfo*)module->getClassInfo(compilingClass);
    args.push_back(LCI->getVar(this));
    Constant* CI = ConstantInt::get(Type::Int32Ty, index);
    args.push_back(CI);
    args.push_back(stat ? mvm::jit::constantOne : mvm::jit::constantZero);
    args.push_back(gvStaticInstance);
    args.push_back(gv);
    Value* tmp = invoke(JnjvmModule::FieldLookupFunction, args, "", currentBlock);
    node->addIncoming(tmp, currentBlock);
    llvm::BranchInst::Create(endBlock, currentBlock);
    
    currentBlock = endBlock;;
    return new BitCastInst(node, fieldTypePtr, "", currentBlock);
  }
}

void JavaJIT::convertValue(Value*& val, const Type* t1, BasicBlock* currentBlock,
                           bool usign) {
  const Type* t2 = val->getType();
  if (t1 != t2) {
    if (t1->isInteger() && t2->isInteger()) {
      if (t2->getPrimitiveSizeInBits() < t1->getPrimitiveSizeInBits()) {
        if (usign) {
          val = new ZExtInst(val, t1, "", currentBlock);
        } else {
          val = new SExtInst(val, t1, "", currentBlock);
        }
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
 

void JavaJIT::setStaticField(uint16 index) {
  const AssessorDesc* ass = topFunc();
  Value* val = pop(); 
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  uint8 id = sign->funcs->numId;
  LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
  const Type* type = LAI.llvmType;
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    val = pop();
  }
  Value* ptr = ldResolved(index, true, 0, type, LAI.llvmTypePtr);
  
  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, 
                 ass == AssessorDesc::dChar || ass == AssessorDesc::dBool);
  }
  
  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getStaticField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  uint8 id = sign->funcs->numId;
  LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
  Value* ptr = ldResolved(index, true, 0, LAI.llvmType, 
                          LAI.llvmTypePtr);
  push(new LoadInst(ptr, "", currentBlock), sign->funcs);
  const Type* type = LAI.llvmType;
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    push(mvm::jit::constantZero, AssessorDesc::dInt);
  }
}

void JavaJIT::setVirtualField(uint16 index) {
  const AssessorDesc* ass = topFunc();
  Value* val = pop();
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  uint8 id = sign->funcs->numId;
  LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
  const Type* type = LAI.llvmType;
  
  if (type == Type::Int64Ty || type == Type::DoubleTy) {
    val = pop();
  }
  
  Value* object = pop();
  JITVerifyNull(object);
  Value* ptr = ldResolved(index, false, object, type, 
                          LAI.llvmTypePtr);

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
  uint8 id = sign->funcs->numId;
  LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
  Value* ptr = ldResolved(index, false, obj, LAI.llvmType, 
                          LAI.llvmTypePtr);
  push(new LoadInst(ptr, "", currentBlock), sign->funcs);
  const Type* type = LAI.llvmType;
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


void JavaJIT::invokeInterfaceOrVirtual(uint16 index) {
  
  // Do the usual
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index);
  
  LLVMSignatureInfo* LSI = module->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  const llvm::PointerType* virtualPtrType = LSI->getVirtualPtrType();

  std::vector<Value*> args; // size = [signature->nbIn + 3];

  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  
  const llvm::Type* retType = virtualType->getReturnType();
  BasicBlock* endBlock = createBasicBlock("end virtual invoke");
  PHINode * node = 0;
  if (retType != Type::VoidTy) {
    node = PHINode::Create(retType, "", endBlock);
  }

  // ok now the cache
  Enveloppe* enveloppe = Enveloppe::allocate(compilingClass->ctpInfo, index);
  compilingMethod->caches.push_back(enveloppe);
  
  Value* zero = mvm::jit::constantZero;
  Value* one = mvm::jit::constantOne;
  Value* two = mvm::jit::constantTwo;
  
  Value* llvmEnv = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (enveloppe)),
                              JnjvmModule::EnveloppeType);
  

  JITVerifyNull(args[0]);

  std::vector<Value*> args1;
  args1.push_back(zero);
  args1.push_back(one);
  Value* cachePtr = GetElementPtrInst::Create(llvmEnv, args1.begin(), args1.end(),
                                          "", currentBlock);
  Value* cache = new LoadInst(cachePtr, "", currentBlock);

  Value* cl = CallInst::Create(JnjvmModule::GetClassFunction, args[0], "",
                               currentBlock);
  std::vector<Value*> args3;
  args3.push_back(zero);
  args3.push_back(two);
  Value* lastCiblePtr = GetElementPtrInst::Create(cache, args3.begin(), 
                                                  args3.end(), "", 
                                                  currentBlock);
  Value* lastCible = new LoadInst(lastCiblePtr, "", currentBlock);

  Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, cl, lastCible, "", currentBlock);
  
  BasicBlock* ifTrue = createBasicBlock("cache ok");
  BasicBlock* ifFalse = createBasicBlock("cache not ok");
  BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
  
  currentBlock = ifFalse;
  Value* _meth = invoke(JnjvmModule::InterfaceLookupFunction, cache, args[0],
                        "", ifFalse);
  Value* meth = new BitCastInst(_meth, virtualPtrType, "", 
                                currentBlock);
  Value* ret = invoke(meth, args, "", currentBlock);
  if (node) {
    node->addIncoming(ret, currentBlock);
  }
  BranchInst::Create(endBlock, currentBlock);

  currentBlock = ifTrue;

  Value* methPtr = GetElementPtrInst::Create(cache, args1.begin(), args1.end(),
                                         "", currentBlock);

  _meth = new LoadInst(methPtr, "", currentBlock);
  meth = new BitCastInst(_meth, virtualPtrType, "", currentBlock);
  
  ret = invoke(meth, args, "", currentBlock);
  BranchInst::Create(endBlock, currentBlock);

  if (node) {
    node->addIncoming(ret, currentBlock);
  }

  currentBlock = endBlock;
  if (node) {
    push(node, signature->ret->funcs);
    if (retType == Type::DoubleTy || retType == Type::Int64Ty) {
      push(mvm::jit::constantZero, AssessorDesc::dInt);
    }
  }
}
