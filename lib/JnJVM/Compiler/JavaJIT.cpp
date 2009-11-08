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

#include <cstring>

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Support/CFG.h>

#include "mvm/JIT.h"

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
#include "Reader.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

using namespace jnjvm;
using namespace llvm;

static bool needsInitialisationCheck(Class* cl, Class* compilingClass) {
#ifdef SERVICE
  return true;
#else

  if (cl->isReadyForCompilation() || 
      (!cl->isInterface() && compilingClass->isAssignableFrom(cl))) {
    return false;
  }

  if (!cl->needsInitialisationCheck()) {
    if (!cl->isReady()) {
      cl->setInitializationState(ready);
    }
    return false;
  }

  return true;
#endif
}

bool JavaJIT::canBeInlined(JavaMethod* meth) {
  JnjvmClassLoader* loader = meth->classDef->classLoader;
  return (meth->canBeInlined &&
          meth != compilingMethod && inlineMethods[meth] == 0 &&
          (loader == compilingClass->classLoader ||
           loader == compilingClass->classLoader->bootstrapLoader));
}

void JavaJIT::invokeVirtual(uint16 index) {
  
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  CommonClass* cl = 0;
  JavaMethod* meth = 0;
  ctpInfo->infoOfMethod(index, ACC_VIRTUAL, cl, meth);
 
  if ((cl && isFinal(cl->access)) || 
      (meth && (isFinal(meth->access) || isPrivate(meth->access)))) {
    return invokeSpecial(index);
  }

  // If the method is in fact a method defined in an interface,
  // call invokeInterface instead.
  if (meth && isInterface(meth->classDef->access)) {
    return invokeInterface(index, true);
  }
 
  const UTF8* name = 0;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index, name);

  Value* obj = objectStack[stack.size() - signature->nbArguments - 1];
  JavaObject* source = TheCompiler->getFinalObject(obj);
  if (source) {
    return invokeSpecial(index, source->getClass());
  }
 
#if !defined(WITHOUT_VTABLE)
  Typedef* retTypedef = signature->getReturnType();
  std::vector<Value*> args; // size = [signature->nbIn + 3];
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbArguments + 1);
  const llvm::Type* retType = virtualType->getReturnType();
   
  JITVerifyNull(args[0]);

  BasicBlock* endBlock = 0;
  PHINode* node = 0;
#if 0
  if (meth && !isAbstract(meth->access)) {
    Value* cl = CallInst::Create(module->GetClassFunction, args[0], "",
                                  currentBlock);
    Value* cl2 = module->getNativeClass(meth->classDef);
    if (cl2->getType() != module->JavaCommonClassType) {
      cl2 = new BitCastInst(cl2, module->JavaCommonClassType, "", currentBlock);
    }

    Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, cl, cl2, "");

    BasicBlock* trueBlock = createBasicBlock("true virtual invoke");
    BasicBlock* falseBlock = createBasicBlock("false virtual invoke");
    endBlock = createBasicBlock("end virtual invoke");
    BranchInst::Create(trueBlock, falseBlock, test, currentBlock);
    currentBlock = trueBlock;
    Value* res = 0;
    if (canBeInlined(meth)) {
      res = invokeInline(meth, args);
    } else {
      Function* func = module->getMethod(meth);
      res = invoke(func, args, "", currentBlock);
    }
    BranchInst::Create(endBlock, currentBlock);
    if (retType != Type::getVoidTy(getGlobalContext())) {
      node = PHINode::Create(virtualType->getReturnType(), "", endBlock);
      node->addIncoming(res, currentBlock);
    }
    currentBlock = falseBlock;
  }
#endif

  Value* VT = CallInst::Create(module->GetVTFunction, args[0], "",
                               currentBlock);
  Value* indexes2[2];
  indexes2[0] = module->constantZero;

#ifdef ISOLATE_SHARING
  Value* indexesCtp; //[3];
#endif
  if (meth) {
    LLVMMethodInfo* LMI = TheCompiler->getMethodInfo(meth);
    Constant* Offset = LMI->getOffset();
    indexes2[1] = Offset;
#ifdef ISOLATE_SHARING
    indexesCtp = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),
                                  Offset->getZExtValue() * -1);
#endif
  } else {
    
    Value* val = getConstantPoolAt(index, module->VirtualLookupFunction,
                                   Type::getInt32Ty(getGlobalContext()), args[0], true);
    indexes2[1] = val;
#ifdef ISOLATE_SHARING
    Value* mul = BinaryOperator::CreateMul(val, module->constantMinusOne,
                                           "", currentBlock);
    indexesCtp = mul;
#endif
  }
  
  Value* FuncPtr = GetElementPtrInst::Create(VT, indexes2, indexes2 + 2, "",
                                             currentBlock);
    
  Value* Func = new LoadInst(FuncPtr, "", currentBlock);
  
  Func = new BitCastInst(Func, LSI->getVirtualPtrType(), "", currentBlock);
#ifdef ISOLATE_SHARING
  Value* CTP = GetElementPtrInst::Create(VT, indexesCtp, "", currentBlock);
    
  CTP = new LoadInst(CTP, "", currentBlock);
  CTP = new BitCastInst(CTP, module->ConstantPoolType, "", currentBlock);
  args.push_back(CTP);
#endif
  Value* val = invoke(Func, args, "", currentBlock);
  
  if (endBlock) {
    if (node) {
      node->addIncoming(val, currentBlock);
      val = node;
    }
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
  }

  if (retType != Type::getVoidTy(getGlobalContext())) {
    push(val, retTypedef->isUnsigned());
    if (retType == Type::getDoubleTy(getGlobalContext()) || retType == Type::getInt64Ty(getGlobalContext())) {
      push(module->constantZero, false);
    }
  }
    
#else
  return invokeInterface(index);
#endif
}

llvm::Value* JavaJIT::getCurrentThread(const llvm::Type* Ty) {
  Value* FrameAddr = CallInst::Create(module->llvm_frameaddress,
                                     	module->constantZero, "", currentBlock);
  Value* threadId = new PtrToIntInst(FrameAddr, module->pointerSizeType, "",
                              			 currentBlock);
  threadId = BinaryOperator::CreateAnd(threadId, module->constantThreadIDMask,
                                       "", currentBlock);
  threadId = new IntToPtrInst(threadId, Ty, "", currentBlock);

  return threadId;
}

extern "C" void jnjvmThrowExceptionFromJIT();

llvm::Function* JavaJIT::nativeCompile(intptr_t natPtr) {
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "native compile %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
  bool stat = isStatic(compilingMethod->access);

  const FunctionType *funcType = llvmFunction->getFunctionType();
  const llvm::Type* returnType = funcType->getReturnType();
  
  bool jnjvm = false;
  
  const UTF8* jniConsClName = compilingClass->name;
  const UTF8* jniConsName = compilingMethod->name;
  const UTF8* jniConsType = compilingMethod->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  char* functionName = (char*)alloca(3 + JNI_NAME_PRE_LEN + 
                            ((mnlen + clen + mtlen) << 3));
  
  if (!natPtr)
    natPtr = compilingClass->classLoader->nativeLookup(compilingMethod, jnjvm,
                                                       functionName);
  
  if (!natPtr && !TheCompiler->isStaticCompiling()) {
    currentBlock = createBasicBlock("start");
    CallInst::Create(module->ThrowExceptionFromJITFunction, "", currentBlock);
    if (returnType != Type::getVoidTy(getGlobalContext()))
      ReturnInst::Create(*llvmContext, Constant::getNullValue(returnType), currentBlock);
    else
      ReturnInst::Create(*llvmContext, currentBlock);
  
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "end native compile %s.%s\n",
                UTF8Buffer(compilingClass->name).cString(),
                UTF8Buffer(compilingMethod->name).cString());
  
    return llvmFunction;
  }
  
  
  Function* func = llvmFunction;
  if (jnjvm) {
    compilingMethod->setCompiledPtr((void*)natPtr, functionName);
    llvmFunction->clearGC();
    return llvmFunction;
  }


  currentExceptionBlock = endExceptionBlock = 0;
  currentBlock = createBasicBlock("start");
  BasicBlock* executeBlock = createBasicBlock("execute");
  endBlock = createBasicBlock("end block");
  
  Constant* sizeB = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), sizeof(jmp_buf));
  Value* oldJB = new AllocaInst(module->ptrType, "", currentBlock);
  Value* newJB = new AllocaInst(Type::getInt8Ty(getGlobalContext()), sizeB, "", currentBlock);
      
  // Allocate currentLocalIndexNumber pointer
  Value* temp = new AllocaInst(Type::getInt32Ty(getGlobalContext()), "",
                               currentBlock);
  new StoreInst(module->constantZero, temp, false, currentBlock);
  
  // Allocate oldCurrentLocalIndexNumber pointer
  Value* oldCLIN = new AllocaInst(PointerType::getUnqual(Type::getInt32Ty(getGlobalContext())), "",
                                  currentBlock);
  
  Constant* sizeF = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 2 * sizeof(void*));
  Value* Frame = new AllocaInst(Type::getInt8Ty(getGlobalContext()), sizeF, "", currentBlock);
  
  // Synchronize before saying we're entering native
  if (isSynchro(compilingMethod->access))
    beginSynchronize();

  Value* test = CallInst::Create(module->setjmpLLVM, newJB, "",
                                 currentBlock);

  test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test,
                      module->constantZero, "");
  BranchInst::Create(executeBlock, endBlock, test, currentBlock);
  
  if (returnType != Type::getVoidTy(getGlobalContext())) {
    endNode = PHINode::Create(returnType, "", endBlock);
    endNode->addIncoming(Constant::getNullValue(returnType),
                         currentBlock);
  }

  currentBlock = executeBlock;
  
  
  uint32 nargs = func->arg_size() + 1 + (stat ? 1 : 0); 
  std::vector<Value*> nativeArgs;
  
  
  Value* threadId = getCurrentThread(module->JavaThreadType);

  Value* geps[2] = { module->constantZero,
                     module->OffsetJNIInThreadConstant };

  Value* jniEnv = GetElementPtrInst::Create(threadId, geps, geps + 2, "",
                                            currentBlock);
 
  jniEnv = new BitCastInst(jniEnv, module->ptrType, "", currentBlock);

  nativeArgs.push_back(jniEnv);

  uint32 index = 0;
  if (stat) {
#ifdef ISOLATE_SHARING
    Value* val = getClassCtp();
    Value* cl = CallInst::Create(module->GetClassDelegateePtrFunction,
                                 val, "", currentBlock);
#else
    Value* cl = TheCompiler->getJavaClassPtr(compilingClass);
#endif
    nativeArgs.push_back(cl);
    index = 2;
  } else {
    index = 1;
  }
  for (Function::arg_iterator i = func->arg_begin(); 
       index < nargs; ++i, ++index) {
    
    if (i->getType() == module->JavaObjectType) {
      BasicBlock* BB = createBasicBlock("");
      BasicBlock* NotZero = createBasicBlock("");
      const Type* Ty = PointerType::getUnqual(module->JavaObjectType);
      PHINode* node = PHINode::Create(Ty, "", BB);

      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, i,
                          module->JavaObjectNullConstant, "");

      node->addIncoming(Constant::getNullValue(Ty), currentBlock);
      BranchInst::Create(BB, NotZero, test, currentBlock);

      currentBlock = NotZero;

      Instruction* temp = new AllocaInst(module->JavaObjectType, "",
                                         func->begin()->begin());
      Value* GCArgs[2] = { 
        new BitCastInst(temp, module->ptrPtrType, "", currentBlock),
        module->constantPtrNull
      };
      
      if (TheCompiler->useCooperativeGC()) {
        CallInst::Create(module->llvm_gc_gcroot, GCArgs, GCArgs + 2, "",
                         temp);
      }
      
      new StoreInst(i, temp, false, currentBlock);
      node->addIncoming(temp, currentBlock);
      BranchInst::Create(BB, currentBlock);

      currentBlock = BB;

      nativeArgs.push_back(node);
    } else {
      nativeArgs.push_back(i);
    }
  }
  
  
  Instruction* ResultObject = 0;
  if (returnType == module->JavaObjectType) {
    ResultObject = new AllocaInst(module->JavaObjectType, "",
                                  func->begin()->begin());
    Value* GCArgs[2] = { 
      new BitCastInst(ResultObject, module->ptrPtrType, "", currentBlock),
      module->constantPtrNull
    };
      
    if (TheCompiler->useCooperativeGC()) {
      CallInst::Create(module->llvm_gc_gcroot, GCArgs, GCArgs + 2, "",
                       currentBlock);
    }
    new StoreInst(module->JavaObjectNullConstant, ResultObject, "",
                  currentBlock);
  }
  
  Value* nativeFunc = TheCompiler->getNativeFunction(compilingMethod,
                                                     (void*)natPtr);

  if (TheCompiler->isStaticCompiling()) {
    Value* Arg = TheCompiler->getMethodInClass(compilingMethod); 
    
    // If the global variable is null, then load it.
    BasicBlock* unloadedBlock = createBasicBlock("");
    BasicBlock* endBlock = createBasicBlock("");
    Value* test = new LoadInst(nativeFunc, "", currentBlock);
    const llvm::Type* Ty = test->getType();
    PHINode* node = PHINode::Create(Ty, "", endBlock);
    node->addIncoming(test, currentBlock);
    Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test,
                              Constant::getNullValue(Ty), "");
    BranchInst::Create(unloadedBlock, endBlock, cmp, currentBlock);
    currentBlock = unloadedBlock;

    Value* res = CallInst::Create(TheCompiler->NativeLoader, Arg, "",
                                  currentBlock);

    res = new BitCastInst(res, Ty, "", currentBlock);
    new StoreInst(res, nativeFunc, currentBlock);
    node->addIncoming(res, currentBlock);
    BranchInst::Create(endBlock, currentBlock);
    currentBlock = endBlock;
    nativeFunc = node;
  }
  
  Value* Args4[5] = { temp, oldCLIN, newJB, oldJB, Frame };

  CallInst::Create(module->StartJNIFunction, Args4, Args4 + 5, "",
                   currentBlock);
  

  // FIXME: Is this still needed?
  //
  // When calling a native method, it may do whatever it wants with the
  // frame pointer. Therefore make sure it's on the stack. x86_64 has
  // this problem because it passes first arguments in registers.
  // Therefore, it was overwriting the frame pointer when entering the
  // native method.
  //Value* FrameAddr = CallInst::Create(module->llvm_frameaddress,
  //                                   	module->constantZero, "", currentBlock);
  //Value* Temp = new AllocaInst(module->ptrType, "", currentBlock);
  //new StoreInst(FrameAddr, Temp, currentBlock);
  
  Value* result = llvm::CallInst::Create(nativeFunc, nativeArgs.begin(),
                                         nativeArgs.end(), "", currentBlock);

  if (returnType == module->JavaObjectType) {
    const Type* Ty = PointerType::getUnqual(module->JavaObjectType);
    Constant* C = Constant::getNullValue(Ty);
    Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, result, C, "");
    BasicBlock* loadBlock = createBasicBlock("");

    endNode->addIncoming(module->JavaObjectNullConstant, currentBlock);
    BranchInst::Create(endBlock, loadBlock, cmp, currentBlock);

    currentBlock = loadBlock;
    result = new LoadInst(result, "", currentBlock);
    new StoreInst(result, ResultObject, "", currentBlock);
    endNode->addIncoming(result, currentBlock);

  } else if (returnType != Type::getVoidTy(getGlobalContext())) {
    endNode->addIncoming(result, currentBlock);
  }
  
  BranchInst::Create(endBlock, currentBlock);


  currentBlock = endBlock; 
 
  Value* Args2[2] = { oldCLIN, oldJB };

  CallInst::Create(module->EndJNIFunction, Args2, Args2 + 2, "", currentBlock);
  
  // Synchronize after leaving native.
  if (isSynchro(compilingMethod->access))
    endSynchronize();
  
  if (returnType != Type::getVoidTy(getGlobalContext()))
    ReturnInst::Create(*llvmContext, endNode, currentBlock);
  else
    ReturnInst::Create(*llvmContext, currentBlock);
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "end native compile %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
  
  return llvmFunction;
}
void JavaJIT::monitorEnter(Value* obj) {
  std::vector<Value*> gep;
  gep.push_back(module->constantZero);
  gep.push_back(module->JavaObjectLockOffsetConstant);
  Value* lockPtr = GetElementPtrInst::Create(obj, gep.begin(), gep.end(), "",
                                             currentBlock);
  
  Value* lock = new LoadInst(lockPtr, "", currentBlock);
  lock = new PtrToIntInst(lock, module->pointerSizeType, "", currentBlock);
  Value* GCMask = ConstantInt::get(module->pointerSizeType,
                                   mvm::GCMask);

  lock = BinaryOperator::CreateAnd(lock, GCMask, "", currentBlock);

  lockPtr = new BitCastInst(lockPtr, 
                            PointerType::getUnqual(module->pointerSizeType),
                            "", currentBlock);
  Value* threadId = getCurrentThread(module->MutatorThreadType);
  threadId = new PtrToIntInst(threadId, module->pointerSizeType, "",
                              currentBlock);
  Value* newValMask = BinaryOperator::CreateOr(threadId, lock, "",
                                               currentBlock);

  std::vector<Value*> atomicArgs;
  atomicArgs.push_back(lockPtr);
  atomicArgs.push_back(lock);
  atomicArgs.push_back(newValMask);

  // Do the atomic compare and swap.
  Value* atomic = CallInst::Create(module->llvm_atomic_lcs_ptr,
                                   atomicArgs.begin(), atomicArgs.end(), "",
                                   currentBlock);
  
  Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, atomic,
                            lock, "");
  
  BasicBlock* OK = createBasicBlock("synchronize passed");
  BasicBlock* NotOK = createBasicBlock("synchronize did not pass");
  BasicBlock* FatLockBB = createBasicBlock("fat lock");
  BasicBlock* ThinLockBB = createBasicBlock("thin lock");

  BranchInst::Create(OK, NotOK, cmp, currentBlock);

  currentBlock = NotOK;
  
  // The compare and swap did not pass, look if it's a thin lock
  Value* isThin = BinaryOperator::CreateAnd(atomic, module->constantFatMask, "",
                                            currentBlock);
  cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, isThin,
                     module->constantPtrZero, "");
  
  BranchInst::Create(ThinLockBB, FatLockBB, cmp, currentBlock);

  // It's a thin lock. Look if we're the owner of this lock.
  currentBlock = ThinLockBB;
  Value* idMask = ConstantInt::get(module->pointerSizeType, mvm::Thread::IDMask);
  Value* cptMask = ConstantInt::get(module->pointerSizeType, mvm::ThinCountMask);
  Value* IdInLock = BinaryOperator::CreateAnd(atomic, idMask, "", currentBlock);
  Value* owner = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, threadId,
                              IdInLock, "");

  BasicBlock* OwnerBB = createBasicBlock("owner thread");

  BranchInst::Create(OwnerBB, FatLockBB, owner, currentBlock);
  currentBlock = OwnerBB;

  // OK, we are the owner, now check if the counter will overflow.
  Value* count = BinaryOperator::CreateAnd(atomic, cptMask, "", currentBlock);
  cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_ULT, count, cptMask, "");

  BasicBlock* IncCounterBB = createBasicBlock("Increment counter");
  BasicBlock* OverflowCounterBB = createBasicBlock("Overflow counter");

  BranchInst::Create(IncCounterBB, OverflowCounterBB, cmp, currentBlock);
  currentBlock = IncCounterBB;
  
  // The counter will not overflow, increment it.
  Value* One = ConstantInt::get(module->pointerSizeType, mvm::ThinCountAdd);
  Value* Add = BinaryOperator::CreateAdd(One, atomic, "", currentBlock);
  new StoreInst(Add, lockPtr, false, currentBlock);
  BranchInst::Create(OK, currentBlock);

  currentBlock = OverflowCounterBB;

  // The counter will overflow, call this function to create a new lock,
  // lock it 0x101 times, and pass.
  CallInst::Create(module->OverflowThinLockFunction, obj, "",
                   currentBlock);
  BranchInst::Create(OK, currentBlock);
  
  currentBlock = FatLockBB;
  // Either it's a fat lock or there is contention.
  CallInst::Create(module->AquireObjectFunction, obj, "", currentBlock);
  BranchInst::Create(OK, currentBlock);
  currentBlock = OK;
}

void JavaJIT::monitorExit(Value* obj) {
  std::vector<Value*> gep;
  gep.push_back(module->constantZero);
  gep.push_back(module->JavaObjectLockOffsetConstant);
  Value* lockPtr = GetElementPtrInst::Create(obj, gep.begin(), gep.end(), "",
                                             currentBlock);
  lockPtr = new BitCastInst(lockPtr, 
                            PointerType::getUnqual(module->pointerSizeType),
                            "", currentBlock);
  Value* lock = new LoadInst(lockPtr, "", currentBlock);
  Value* GCMask = ConstantInt::get(module->pointerSizeType, ~mvm::GCMask);

  Value* lockedMask = BinaryOperator::CreateAnd(lock, GCMask, "", currentBlock);
  
  Value* threadId = getCurrentThread(module->MutatorThreadType);
  threadId = new PtrToIntInst(threadId, module->pointerSizeType, "",
                              currentBlock);
  
  Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, lockedMask,
                            threadId, "");
  
  
  BasicBlock* EndUnlock = createBasicBlock("end unlock");
  BasicBlock* LockedOnceBB = createBasicBlock("desynchronize thin lock");
  BasicBlock* NotLockedOnceBB = 
    createBasicBlock("simple desynchronize did not pass");
  BasicBlock* FatLockBB = createBasicBlock("fat lock");
  BasicBlock* ThinLockBB = createBasicBlock("thin lock");
  
  BranchInst::Create(LockedOnceBB, NotLockedOnceBB, cmp, currentBlock);
  
  // Locked once, set zero
  currentBlock = LockedOnceBB;
  GCMask = ConstantInt::get(module->pointerSizeType, mvm::GCMask);
  lockedMask = BinaryOperator::CreateAnd(lock, GCMask, "", currentBlock);
  new StoreInst(lockedMask, lockPtr, false, currentBlock);
  BranchInst::Create(EndUnlock, currentBlock);

  currentBlock = NotLockedOnceBB;
  // Look if the lock is thin.
  Value* isThin = BinaryOperator::CreateAnd(lock, module->constantFatMask, "",
                                            currentBlock);
  cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, isThin,
                     module->constantPtrZero, "");
  
  BranchInst::Create(ThinLockBB, FatLockBB, cmp, currentBlock);
  
  currentBlock = ThinLockBB;

  // Decrement the counter.
  Value* One = ConstantInt::get(module->pointerSizeType, mvm::ThinCountAdd);
  Value* Sub = BinaryOperator::CreateSub(lock, One, "", currentBlock);
  new StoreInst(Sub, lockPtr, false, currentBlock);
  BranchInst::Create(EndUnlock, currentBlock);

  currentBlock = FatLockBB;

  // Either it's a fat lock or there is contention.
  CallInst::Create(module->ReleaseObjectFunction, obj, "", currentBlock);
  BranchInst::Create(EndUnlock, currentBlock);
  currentBlock = EndUnlock;
}



#ifdef ISOLATE_SHARING
Value* JavaJIT::getStaticInstanceCtp() {
  Value* cl = getClassCtp();
  Value* indexes[2] = { module->constantZero, module->constantSeven };
  Value* arg1 = GetElementPtrInst::Create(cl, indexes, indexes + 2,
                                          "", currentBlock);
  arg1 = new LoadInst(arg1, "", false, currentBlock);
  return arg1;
  
}

Value* JavaJIT::getClassCtp() {
  Value* indexes = module->constantOne;
  Value* arg1 = GetElementPtrInst::Create(ctpCache, indexes.begin(),
                                          indexes.end(),  "", currentBlock);
  arg1 = new LoadInst(arg1, "", false, currentBlock);
  arg1 = new BitCastInst(arg1, module->JavaClassType, "", currentBlock);
  return arg1;
}
#endif

void JavaJIT::beginSynchronize() {
  Value* obj = 0;
  if (isVirtual(compilingMethod->access)) {
    obj = llvmFunction->arg_begin();
  } else {
    obj = TheCompiler->getJavaClass(compilingClass);
  }
  monitorEnter(obj);
}

void JavaJIT::endSynchronize() {
  Value* obj = 0;
  if (isVirtual(compilingMethod->access)) {
    obj = llvmFunction->arg_begin();
  } else {
    obj = TheCompiler->getJavaClass(compilingClass);
  }
  monitorExit(obj);
}


static void removeUnusedLocals(std::vector<AllocaInst*>& locals) {
  for (std::vector<AllocaInst*>::iterator i = locals.begin(),
       e = locals.end(); i != e; ++i) {
    AllocaInst* temp = *i;
    unsigned uses = temp->getNumUses();
    if (!uses) {
      temp->eraseFromParent();
    } else if (uses == 1 && dyn_cast<StoreInst>(temp->use_begin())) {
      dyn_cast<StoreInst>(temp->use_begin())->eraseFromParent();
      temp->eraseFromParent();
    }
  }
}
  
static void removeUnusedObjects(std::vector<AllocaInst*>& objects,
                                JnjvmModule* module, bool coop) {
  for (std::vector<AllocaInst*>::iterator i = objects.begin(),
       e = objects.end(); i != e; ++i) {
    AllocaInst* temp = *i;
    unsigned uses = temp->getNumUses();
    if (!uses) {
      temp->eraseFromParent();
    } else if (uses == 1 && dyn_cast<StoreInst>(temp->use_begin())) {
      dyn_cast<StoreInst>(temp->use_begin())->eraseFromParent();
      temp->eraseFromParent();
    } else {
      if (coop) {
        Instruction* I = new BitCastInst(temp, module->ptrPtrType, "");
        I->insertAfter(temp);
        Value* GCArgs[2] = { I, module->constantPtrNull };
        Instruction* C = CallInst::Create(module->llvm_gc_gcroot, GCArgs,
                                          GCArgs + 2, "");
        C->insertAfter(I);
      }
    }
  }
}

Instruction* JavaJIT::inlineCompile(BasicBlock*& curBB,
                                    BasicBlock* endExBlock,
                                    std::vector<Value*>& args) {
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "inline compile %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
  Attribut* codeAtt = compilingMethod->lookupAttribut(Attribut::codeAttribut);
  
  if (!codeAtt) {
    fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " no code attribut found for %s.%s!\n",
                    UTF8Buffer(compilingClass->name).cString(),
                    UTF8Buffer(compilingMethod->name).cString());
    abort();
  }

  Reader reader(codeAtt, &(compilingClass->bytes));
  uint16 maxStack = reader.readU2();
  uint16 maxLocals = reader.readU2();
  uint32 codeLen = reader.readU4();
  uint32 start = reader.cursor;
  
  reader.seek(codeLen, Reader::SeekCur);
  
  LLVMMethodInfo* LMI = TheCompiler->getMethodInfo(compilingMethod);
  assert(LMI);
  Function* func = LMI->getMethod();

  const Type* returnType = func->getReturnType();
  endBlock = createBasicBlock("end");

  currentBlock = curBB;
  endExceptionBlock = endExBlock;

  opcodeInfos = (Opinfo*)alloca(codeLen * sizeof(Opinfo));
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExBlock;
  }

  BasicBlock* firstBB = llvmFunction->begin();
  
  if (firstBB->begin() != firstBB->end()) {
    Instruction* firstInstruction = firstBB->begin();

    for (int i = 0; i < maxLocals; i++) {
      intLocals.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, firstInstruction);
      doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "",
                                            firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, firstInstruction);
      longLocals.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, firstInstruction);
      floatLocals.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, firstInstruction);
      objectLocals.push_back(new AllocaInst(module->JavaObjectType, "",
                                          firstInstruction));
      new StoreInst(Constant::getNullValue(module->JavaObjectType), objectLocals.back(), false, firstInstruction);
    }
    for (int i = 0; i < maxStack; i++) {
      objectStack.push_back(new AllocaInst(module->JavaObjectType, "",
                                           firstInstruction));
      new StoreInst(Constant::getNullValue(module->JavaObjectType), objectStack.back(), false, firstInstruction);
      intStack.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", firstInstruction));
      doubleStack.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "",
                                           firstInstruction));
      longStack.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", firstInstruction));
      floatStack.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", firstInstruction));
    }

  } else {
    for (int i = 0; i < maxLocals; i++) {
      intLocals.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, firstBB);
      doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, firstBB);
      longLocals.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, firstBB);
      floatLocals.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, firstBB);
      objectLocals.push_back(new AllocaInst(module->JavaObjectType, "",
                                            firstBB));
      new StoreInst(Constant::getNullValue(module->JavaObjectType), objectLocals.back(), false, firstBB);
    }
    
    for (int i = 0; i < maxStack; i++) {
      objectStack.push_back(new AllocaInst(module->JavaObjectType, "",
                                           firstBB));
      new StoreInst(Constant::getNullValue(module->JavaObjectType), objectStack.back(), false, firstBB);
      intStack.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", firstBB));
      doubleStack.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "", firstBB));
      longStack.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", firstBB));
      floatStack.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", firstBB));
    }
  }
      
  
  uint32 index = 0;
  uint32 count = 0;
#if defined(ISOLATE_SHARING)
  uint32 max = args.size() - 2;
#else
  uint32 max = args.size();
#endif
  Signdef* sign = compilingMethod->getSignature();
  Typedef* const* arguments = sign->getArgumentsType();
  uint32 type = 0;
  std::vector<Value*>::iterator i = args.begin(); 

  if (isVirtual(compilingMethod->access)) {
    new StoreInst(*i, objectLocals[0], false, currentBlock);
    ++i;
    ++index;
    ++count;
  }

  
  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const Typedef* cur = arguments[type];
    const Type* curType = (*i)->getType();

    if (curType == Type::getInt64Ty(getGlobalContext())){
      new StoreInst(*i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur->isUnsigned()) {
      new StoreInst(new ZExtInst(*i, Type::getInt32Ty(getGlobalContext()), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt8Ty(getGlobalContext()) || curType == Type::getInt16Ty(getGlobalContext())) {
      new StoreInst(new SExtInst(*i, Type::getInt32Ty(getGlobalContext()), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt32Ty(getGlobalContext())) {
      new StoreInst(*i, intLocals[index], false, currentBlock);
    } else if (curType == Type::getDoubleTy(getGlobalContext())) {
      new StoreInst(*i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (curType == Type::getFloatTy(getGlobalContext())) {
      new StoreInst(*i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(*i, objectLocals[index], false, currentBlock);
    }
  }
  
  readExceptionTable(reader, codeLen);

  exploreOpcodes(&compilingClass->bytes->elements[start], codeLen);
  nbEnveloppes = 0;

  if (returnType != Type::getVoidTy(getGlobalContext())) {
    endNode = PHINode::Create(returnType, "", endBlock);
  }

  compileOpcodes(&compilingClass->bytes->elements[start], codeLen);
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL,
              "--> end inline compiling %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());

  curBB = endBlock;


  removeUnusedLocals(intLocals);
  removeUnusedLocals(doubleLocals);
  removeUnusedLocals(floatLocals);
  removeUnusedLocals(longLocals);
  removeUnusedLocals(intStack);
  removeUnusedLocals(doubleStack);
  removeUnusedLocals(floatStack);
  removeUnusedLocals(longStack);
  
  removeUnusedObjects(objectLocals, module, TheCompiler->useCooperativeGC());
  removeUnusedObjects(objectStack, module, TheCompiler->useCooperativeGC());


  return endNode;
    
}

llvm::Function* JavaJIT::javaCompile() {
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "compiling %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());

  
  Attribut* codeAtt = compilingMethod->lookupAttribut(Attribut::codeAttribut);
  
  if (!codeAtt) {
    fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " no code attribut found for %s.%s!\n",
                    UTF8Buffer(compilingClass->name).cString(),
                    UTF8Buffer(compilingMethod->name).cString());
    abort();
  }

  Reader reader(codeAtt, &(compilingClass->bytes));
  uint16 maxStack = reader.readU2();
  uint16 maxLocals = reader.readU2();
  uint32 codeLen = reader.readU4();
  uint32 start = reader.cursor;
  
  reader.seek(codeLen, Reader::SeekCur);

  const FunctionType *funcType = llvmFunction->getFunctionType();
  const Type* returnType = funcType->getReturnType();
  
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
    Value* arg = TheCompiler->getMethodInClass(compilingMethod);

    llvm::CallInst::Create(module->PrintMethodStartFunction, arg, "",
                           currentBlock);
    }
#endif

  

  for (int i = 0; i < maxLocals; i++) {
    intLocals.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, currentBlock);
    doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, currentBlock);
    longLocals.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, currentBlock);
    floatLocals.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, currentBlock);
    objectLocals.push_back(new AllocaInst(module->JavaObjectType, "",
                                          currentBlock));
    new StoreInst(Constant::getNullValue(module->JavaObjectType), objectLocals.back(), false, currentBlock);
  }
  
  for (int i = 0; i < maxStack; i++) {
    objectStack.push_back(new AllocaInst(module->JavaObjectType, "",
                                         currentBlock));
    new StoreInst(Constant::getNullValue(module->JavaObjectType), objectStack.back(), false, currentBlock);
    intStack.push_back(new AllocaInst(Type::getInt32Ty(getGlobalContext()), "", currentBlock));
    doubleStack.push_back(new AllocaInst(Type::getDoubleTy(getGlobalContext()), "", currentBlock));
    longStack.push_back(new AllocaInst(Type::getInt64Ty(getGlobalContext()), "", currentBlock));
    floatStack.push_back(new AllocaInst(Type::getFloatTy(getGlobalContext()), "", currentBlock));
  }
  
  uint32 index = 0;
  uint32 count = 0;
#if defined(ISOLATE_SHARING)
  uint32 max = func->arg_size() - 2;
#else
  uint32 max = func->arg_size();
#endif
  Function::arg_iterator i = func->arg_begin(); 
  Signdef* sign = compilingMethod->getSignature();
  Typedef* const* arguments = sign->getArgumentsType();
  uint32 type = 0;

  if (isVirtual(compilingMethod->access)) {
    new StoreInst(i, objectLocals[0], false, currentBlock);
    ++i;
    ++index;
    ++count;
  }

  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const Typedef* cur = arguments[type];
    const llvm::Type* curType = i->getType();

    if (curType == Type::getInt64Ty(getGlobalContext())){
      new StoreInst(i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur->isUnsigned()) {
      new StoreInst(new ZExtInst(i, Type::getInt32Ty(getGlobalContext()), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt8Ty(getGlobalContext()) || curType == Type::getInt16Ty(getGlobalContext())) {
      new StoreInst(new SExtInst(i, Type::getInt32Ty(getGlobalContext()), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt32Ty(getGlobalContext())) {
      new StoreInst(i, intLocals[index], false, currentBlock);
    } else if (curType == Type::getDoubleTy(getGlobalContext())) {
      new StoreInst(i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (curType == Type::getFloatTy(getGlobalContext())) {
      new StoreInst(i, floatLocals[index], false, currentBlock);
    } else {
      new StoreInst(i, objectLocals[index], false, currentBlock);
    }
  }

#if defined(ISOLATE_SHARING)
  ctpCache = i;
  Value* addrCtpCache = new AllocaInst(module->ConstantPoolType, "",
                                       currentBlock);
  /// make it volatile to be sure it's on the stack
  new StoreInst(ctpCache, addrCtpCache, true, currentBlock);
#endif
 

#if defined(SERVICE)
  JnjvmClassLoader* loader = compilingClass->classLoader;
  Value* Cmp = 0;
  Value* threadId = 0;
  Value* OldIsolateID = 0;
  Value* IsolateIDPtr = 0;
  Value* OldIsolate = 0;
  Value* NewIsolate = 0;
  Value* IsolatePtr = 0;
  if (loader != loader->bootstrapLoader && isPublic(compilingMethod->access)) {
    threadId = getCurrentThread(module->MutatorThreadType);
     
    IsolateIDPtr = GetElementPtrInst::Create(threadId, module->constantThree,
                                             "", currentBlock);
    const Type* realType = PointerType::getUnqual(module->pointerSizeType);
    IsolateIDPtr = new BitCastInst(IsolateIDPtr, realType, "",
                                   currentBlock);
    OldIsolateID = new LoadInst(IsolateIDPtr, "", currentBlock);

    Value* MyID = ConstantInt::get(module->pointerSizeType,
                                   loader->getIsolate()->IsolateID);
    Cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, OldIsolateID, MyID,
                       "");

    BasicBlock* EndBB = createBasicBlock("After service check");
    BasicBlock* ServiceBB = createBasicBlock("Begin service call");

    BranchInst::Create(EndBB, ServiceBB, Cmp, currentBlock);

    currentBlock = ServiceBB;
  
    new StoreInst(MyID, IsolateIDPtr, currentBlock);
    IsolatePtr = GetElementPtrInst::Create(threadId, module->constantFour, "",
                                           currentBlock);
     
    OldIsolate = new LoadInst(IsolatePtr, "", currentBlock);
    NewIsolate = module->getIsolate(loader->getIsolate(), currentBlock);
    new StoreInst(NewIsolate, IsolatePtr, currentBlock);

#if DEBUG
    Value* GEP[2] = { OldIsolate, NewIsolate };
    CallInst::Create(module->ServiceCallStartFunction, GEP, GEP + 2,
                     "", currentBlock);
#endif
    BranchInst::Create(EndBB, currentBlock);
    currentBlock = EndBB;
  }
#endif

  readExceptionTable(reader, codeLen);
  
  exploreOpcodes(&compilingClass->bytes->elements[start], codeLen);
  compilingMethod->enveloppes = 
    new (compilingClass->classLoader->allocator, "Enveloppes")
    Enveloppe[nbEnveloppes];
  compilingMethod->nbEnveloppes = nbEnveloppes;
  nbEnveloppes = 0;
 
  endBlock = createBasicBlock("end");

  if (returnType != Type::getVoidTy(getGlobalContext())) {
    endNode = llvm::PHINode::Create(returnType, "", endBlock);
  }
  
  if (isSynchro(compilingMethod->access))
    beginSynchronize();
 
  if (TheCompiler->hasExceptionsEnabled()) {
    // Variables have been allocated and the lock has been taken. Do the stack
    // check now: if there is an exception, we will go to the lock release code.
    currentExceptionBlock = opcodeInfos[0].exceptionBlock;
    Value* FrameAddr = CallInst::Create(module->llvm_frameaddress,
                                       	module->constantZero, "", currentBlock);
    FrameAddr = new PtrToIntInst(FrameAddr, module->pointerSizeType, "",
                                 currentBlock);
    Value* stackCheck = 
      BinaryOperator::CreateAnd(FrameAddr, module->constantStackOverflowMask,
                                "", currentBlock);

    stackCheck = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, stackCheck,
                              module->constantPtrZero, "");
    BasicBlock* stackOverflow = createBasicBlock("stack overflow");
    BasicBlock* noStackOverflow = createBasicBlock("no stack overflow");
    BranchInst::Create(stackOverflow, noStackOverflow, stackCheck,
                       currentBlock);
    currentBlock = stackOverflow;
    throwException(module->StackOverflowErrorFunction, 0, 0);
    currentBlock = noStackOverflow;
  }
  
  if (TheCompiler->useCooperativeGC()) {
    Value* threadId = getCurrentThread(module->MutatorThreadType);
    
    Value* GEP[2] = { module->constantZero, 
                      module->OffsetDoYieldInThreadConstant };
    
    Value* YieldPtr = GetElementPtrInst::Create(threadId, GEP, GEP + 2, "",
                                                currentBlock);

    Value* Yield = new LoadInst(YieldPtr, "", currentBlock);

    BasicBlock* continueBlock = createBasicBlock("After safe point");
    BasicBlock* yieldBlock = createBasicBlock("In safe point");
    BranchInst::Create(yieldBlock, continueBlock, Yield, currentBlock);

    currentBlock = yieldBlock;
    CallInst::Create(module->conditionalSafePoint, "", currentBlock);
    BranchInst::Create(continueBlock, currentBlock);

    currentBlock = continueBlock;
  }

  compileOpcodes(&compilingClass->bytes->elements[start], codeLen); 
  
  assert(stack.size() == 0 && "Stack not empty after compiling bytecode");
  // Fix a javac(?) bug where a method only throws an exception and does
  // not return.
  pred_iterator PI = pred_begin(endBlock);
  pred_iterator PE = pred_end(endBlock);
  if (PI == PE && returnType != Type::getVoidTy(getGlobalContext())) {
    Instruction* I = currentBlock->getTerminator();
    
    if (isa<UnreachableInst>(I)) {
      I->eraseFromParent();
      BranchInst::Create(endBlock, currentBlock);
      endNode->addIncoming(Constant::getNullValue(returnType),
                           currentBlock);
    } else if (InvokeInst* II = dyn_cast<InvokeInst>(I)) {
      II->setNormalDest(endBlock);
      endNode->addIncoming(Constant::getNullValue(returnType),
                           currentBlock);
    }

  }
  currentBlock = endBlock;

  if (isSynchro(compilingMethod->access))
    endSynchronize();

#if JNJVM_EXECUTE > 0
    {
    Value* arg = TheCompiler->getMethodInClass(compilingMethod); 
    CallInst::Create(module->PrintMethodEndFunction, arg, "", currentBlock);
    }
#endif
  
#if defined(SERVICE)
  if (Cmp) {
    BasicBlock* EndBB = createBasicBlock("After service check");
    BasicBlock* ServiceBB = createBasicBlock("End Service call");

    BranchInst::Create(EndBB, ServiceBB, Cmp, currentBlock);

    currentBlock = ServiceBB;
  
    new StoreInst(OldIsolateID, IsolateIDPtr, currentBlock);
    new StoreInst(OldIsolate, IsolatePtr, currentBlock);

#if DEBUG
    Value* GEP[2] = { OldIsolate, NewIsolate };
    CallInst::Create(module->ServiceCallStopFunction, GEP, GEP + 2,
                     "", currentBlock);
#endif
    BranchInst::Create(EndBB, currentBlock);
    currentBlock = EndBB;
  }
#endif

  PI = pred_begin(currentBlock);
  PE = pred_end(currentBlock);
  if (PI == PE) {
    currentBlock->eraseFromParent();
  } else {
    if (returnType != Type::getVoidTy(getGlobalContext()))
      ReturnInst::Create(*llvmContext, endNode, currentBlock);
    else
      ReturnInst::Create(*llvmContext, currentBlock);
  }

  currentBlock = endExceptionBlock;

  finishExceptions();
   
  removeUnusedLocals(intLocals);
  removeUnusedLocals(doubleLocals);
  removeUnusedLocals(floatLocals);
  removeUnusedLocals(longLocals);
  removeUnusedLocals(intStack);
  removeUnusedLocals(doubleStack);
  removeUnusedLocals(floatStack);
  removeUnusedLocals(longStack);
  
  removeUnusedObjects(objectLocals, module, TheCompiler->useCooperativeGC());
  removeUnusedObjects(objectStack, module, TheCompiler->useCooperativeGC());
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "--> end compiling %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
#ifndef DWARF_EXCEPTIONS
  if (codeLen < 5 && !callsStackWalker && !TheCompiler->isStaticCompiling())
    compilingMethod->canBeInlined = true;
#endif

  return llvmFunction;
}

void JavaJIT::compareFP(Value* val1, Value* val2, const Type* ty, bool l) {
  Value* one = module->constantOne;
  Value* zero = module->constantZero;
  Value* minus = module->constantMinusOne;

  Value* c = new FCmpInst(*currentBlock, FCmpInst::FCMP_UGT, val1, val2, "");
  Value* r = llvm::SelectInst::Create(c, one, zero, "", currentBlock);
  c = new FCmpInst(*currentBlock, FCmpInst::FCMP_ULT, val1, val2, "");
  r = llvm::SelectInst::Create(c, minus, r, "", currentBlock);
  c = new FCmpInst(*currentBlock, FCmpInst::FCMP_UNO, val1, val2, "");
  r = llvm::SelectInst::Create(c, l ? one : minus, r, "", currentBlock);

  push(r, false);

}

void JavaJIT::loadConstant(uint16 index) {
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  uint8 type = ctpInfo->typeAt(index);
  
  if (type == JavaConstantPool::ConstantString) {
#if defined(ISOLATE)
    abort();
#else
    
    if (TheCompiler->isStaticCompiling()) {
      const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[index]);
      JavaString* str = *(compilingClass->classLoader->UTF8ToStr(utf8));
      Value* val = TheCompiler->getString(str);
      push(val, false);
    } else {
      JavaString** str = (JavaString**)ctpInfo->ctpRes[index];
      if (str) {
        Value* val = TheCompiler->getStringPtr(str);
        val = new LoadInst(val, "", currentBlock);
        push(val, false);
      } else {
        // Lookup the constant pool cache
        const llvm::Type* Ty = PointerType::getUnqual(module->JavaObjectType);
        Value* val = getConstantPoolAt(index, module->StringLookupFunction,
                                       Ty, 0, false);
        val = new LoadInst(val, "", currentBlock);
        push(val, false);
      }
    }
#endif   
  } else if (type == JavaConstantPool::ConstantLong) {
    push(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), ctpInfo->LongAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantDouble) {
    push(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), ctpInfo->DoubleAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantInteger) {
    push(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), ctpInfo->IntegerAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantFloat) {
    push(ConstantFP::get(Type::getFloatTy(getGlobalContext()), ctpInfo->FloatAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantClass) {
    UserCommonClass* cl = 0;
    Value* res = getResolvedCommonClass(index, false, &cl);

    res = CallInst::Create(module->GetClassDelegateeFunction, res, "",
                           currentBlock);
    push(res, false);
  } else {
    fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " unknown ldc %d in %s.%s!\n", type,
                    UTF8Buffer(compilingClass->name).cString(),
                    UTF8Buffer(compilingMethod->name).cString());
    abort();
  }
}

void JavaJIT::JITVerifyNull(Value* obj) {

  if (TheCompiler->hasExceptionsEnabled()) {
    Constant* zero = module->JavaObjectNullConstant;
    Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, obj, zero, "");

    BasicBlock* exit = createBasicBlock("verifyNullExit");
    BasicBlock* cont = createBasicBlock("verifyNullCont");

    BranchInst::Create(exit, cont, test, currentBlock);
    currentBlock = exit;
    throwException(module->NullPointerExceptionFunction, 0, 0);
    currentBlock = cont;
  }
 
}

Value* JavaJIT::verifyAndComputePtr(Value* obj, Value* index,
                                    const Type* arrayType, bool verif) {
  JITVerifyNull(obj);
  
  if (index->getType() != Type::getInt32Ty(getGlobalContext())) {
    index = new SExtInst(index, Type::getInt32Ty(getGlobalContext()), "", currentBlock);
  }
  
  if (TheCompiler->hasExceptionsEnabled()) {
    Value* size = arraySize(obj);
    
    Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_ULT, index, size,
                              "");

    BasicBlock* ifTrue =  createBasicBlock("true verifyAndComputePtr");
    BasicBlock* ifFalse = createBasicBlock("false verifyAndComputePtr");

    BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
    
    currentBlock = ifFalse;
    Value* args[2] = { obj, index };
    throwException(module->IndexOutOfBoundsExceptionFunction, args, 2);
    currentBlock = ifTrue;
  }
  
  Constant* zero = module->constantZero;
  Value* val = new BitCastInst(obj, arrayType, "", currentBlock);
  
  Value* indexes[3] = { zero, module->JavaArrayElementsOffsetConstant, index };
  Value* ptr = GetElementPtrInst::Create(val, indexes, indexes + 3, "",
                                         currentBlock);

  return ptr;

}

void JavaJIT::makeArgs(FunctionType::param_iterator it,
                       uint32 index, std::vector<Value*>& Args, uint32 nb) {
#if defined(ISOLATE_SHARING)
  nb += 1;
#endif
  Args.reserve(nb + 2);
  Value** args = (Value**)alloca(nb*sizeof(Value*));
#if defined(ISOLATE_SHARING)
  args[nb - 1] = isolateLocal;
  sint32 start = nb - 2;
  it--;
  it--;
#else
  sint32 start = nb - 1;
#endif
  for (sint32 i = start; i >= 0; --i) {
    it--;
    if (it->get() == Type::getInt64Ty(getGlobalContext()) || it->get() == Type::getDoubleTy(getGlobalContext())) {
      pop();
    }
    bool unsign = topIsUnsigned();
    Value* tmp = pop();
    
    const Type* type = it->get();
    if (tmp->getType() != type) { // int8 or int16
      convertValue(tmp, type, currentBlock, unsign);
    }
    args[i] = tmp;

  }

  for (uint32 i = 0; i < nb; ++i) {
    Args.push_back(args[i]);
  }
  
}

void JavaJIT::addFakePHINodes(BasicBlock* dest, BasicBlock* insert) {
  if(dest->empty()) {
    for (std::vector<StackTypeInfo>::iterator i = stack.begin(),
         e = stack.end(); i!= e; ++i) {
      switch (*i) {
        case Int : {
          PHINode* node = PHINode::Create(Type::getInt32Ty(getGlobalContext()), "", dest);
          node->addIncoming(Constant::getNullValue(Type::getInt32Ty(getGlobalContext())), insert);
          break;
        }
        case Float : {
          PHINode* node = PHINode::Create(Type::getFloatTy(getGlobalContext()), "", dest);
          node->addIncoming(Constant::getNullValue(Type::getFloatTy(getGlobalContext())), insert);
          break;
        }
        case Double : {
          PHINode* node = PHINode::Create(Type::getDoubleTy(getGlobalContext()), "", dest);
          node->addIncoming(Constant::getNullValue(Type::getDoubleTy(getGlobalContext())), insert);
          break;
        }
        case Long : {
          PHINode* node = PHINode::Create(Type::getInt64Ty(getGlobalContext()), "", dest);
          node->addIncoming(Constant::getNullValue(Type::getInt64Ty(getGlobalContext())), insert);
          break;
        }
        case Object : {
          PHINode* node = PHINode::Create(module->JavaObjectType, "", dest);
          node->addIncoming(Constant::getNullValue(module->JavaObjectType),
                            insert);
          break;
        }
        default :
          abort();
      }
    }
  } else {
    for (BasicBlock::iterator i = dest->begin(), e = dest->end(); i != e; ++i) {
      if (PHINode* node = dyn_cast<PHINode>(i)) {
        node->addIncoming(Constant::getNullValue(node->getType()), insert);
      } else {
        break;
      }
    }
  }
}


Instruction* JavaJIT::lowerMathOps(const UTF8* name, 
                                   std::vector<Value*>& args) {
  JnjvmBootstrapLoader* loader = compilingClass->classLoader->bootstrapLoader;
  if (name->equals(loader->abs)) {
    const Type* Ty = args[0]->getType();
    if (Ty == Type::getInt32Ty(getGlobalContext())) {
      Constant* const_int32_9 = module->constantZero;
      Constant* const_int32_10 = module->constantMinusOne;
      BinaryOperator* int32_tmpneg = 
        BinaryOperator::Create(Instruction::Sub, const_int32_9, args[0],
                               "tmpneg", currentBlock);
      ICmpInst* int1_abscond = 
        new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT, args[0], const_int32_10,
                     "abscond");
      return llvm::SelectInst::Create(int1_abscond, args[0], int32_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::getInt64Ty(getGlobalContext())) {
      Constant* const_int64_9 = module->constantLongZero;
      Constant* const_int64_10 = module->constantLongMinusOne;
      
      BinaryOperator* int64_tmpneg = 
        BinaryOperator::Create(Instruction::Sub, const_int64_9, args[0],
                               "tmpneg", currentBlock);

      ICmpInst* int1_abscond = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT,
                                            args[0], const_int64_10, "abscond");
      
      return llvm::SelectInst::Create(int1_abscond, args[0], int64_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::getFloatTy(getGlobalContext())) {
      return llvm::CallInst::Create(module->func_llvm_fabs_f32, args[0],
                                    "tmp1", currentBlock);
    } else if (Ty == Type::getDoubleTy(getGlobalContext())) {
      return llvm::CallInst::Create(module->func_llvm_fabs_f64, args[0],
                                    "tmp1", currentBlock);
    }
  } else if (name->equals(loader->sqrt)) {
    return llvm::CallInst::Create(module->func_llvm_sqrt_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->sin)) {
    return llvm::CallInst::Create(module->func_llvm_sin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->cos)) {
    return llvm::CallInst::Create(module->func_llvm_cos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->tan)) {
    return llvm::CallInst::Create(module->func_llvm_tan_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->asin)) {
    return llvm::CallInst::Create(module->func_llvm_asin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->acos)) {
    return llvm::CallInst::Create(module->func_llvm_acos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->atan)) {
    return llvm::CallInst::Create(module->func_llvm_atan_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->atan2)) {
    return llvm::CallInst::Create(module->func_llvm_atan2_f64, 
                                  args.begin(), args.end(), "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->exp)) {
    return llvm::CallInst::Create(module->func_llvm_exp_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log)) {
    return llvm::CallInst::Create(module->func_llvm_log_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->pow)) {
    return llvm::CallInst::Create(module->func_llvm_pow_f64, args.begin(),
                                  args.end(), "tmp1", currentBlock);
  } else if (name->equals(loader->ceil)) {
    return llvm::CallInst::Create(module->func_llvm_ceil_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->floor)) {
    return llvm::CallInst::Create(module->func_llvm_floor_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->rint)) {
    return llvm::CallInst::Create(module->func_llvm_rint_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->cbrt)) {
    return llvm::CallInst::Create(module->func_llvm_cbrt_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->cosh)) {
    return llvm::CallInst::Create(module->func_llvm_cosh_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->expm1)) {
    return llvm::CallInst::Create(module->func_llvm_expm1_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->hypot)) {
    return llvm::CallInst::Create(module->func_llvm_hypot_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log10)) {
    return llvm::CallInst::Create(module->func_llvm_log10_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log1p)) {
    return llvm::CallInst::Create(module->func_llvm_log1p_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->sinh)) {
    return llvm::CallInst::Create(module->func_llvm_sinh_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->tanh)) {
    return llvm::CallInst::Create(module->func_llvm_tanh_f64, args[0],
                                  "tmp1", currentBlock);
  }
  
  return 0;

}


Instruction* JavaJIT::invokeInline(JavaMethod* meth, 
                                   std::vector<Value*>& args) {
  JavaJIT jit(TheCompiler, meth, llvmFunction);
  jit.unifiedUnreachable = unifiedUnreachable;
  jit.inlineMethods = inlineMethods;
  jit.inlineMethods[meth] = true;
  jit.inlining = true;
  Instruction* ret = jit.inlineCompile(currentBlock, 
                                       currentExceptionBlock, args);
  inlineMethods[meth] = false;
  return ret;
}

void JavaJIT::invokeSpecial(uint16 index, CommonClass* finalCl) {
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  JavaMethod* meth = 0;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* cl = 0;

  ctpInfo->nameOfStaticOrSpecialMethod(index, cl, name, signature);
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  llvm::Instruction* val = 0;
  
  std::vector<Value*> args;
  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbArguments + 1);
  Value* func = 0;

  if (finalCl) {
    Class* lookup = finalCl->isArray() ? finalCl->super : finalCl->asClass();

    meth = lookup->lookupMethodDontThrow(name, signature->keyName, false, true,
                                         0);
  }
  
  if (!meth) {
    meth = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_VIRTUAL, signature);
  }
  
  if (meth == compilingClass->classLoader->bootstrapLoader->upcalls->InitObject)
    return;
  
  JITVerifyNull(args[0]); 

#if defined(ISOLATE_SHARING)
  const Type* Ty = module->ConstantPoolType;
  Constant* Nil = Constant::getNullValue(Ty);
  GlobalVariable* GV = new GlobalVariable(Ty, false,
                                          GlobalValue::ExternalLinkage, Nil,
                                          "", module);
  Value* res = new LoadInst(GV, "", false, currentBlock);
  Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, res, Nil, "");
 
  BasicBlock* trueCl = createBasicBlock("UserCtp OK");
  BasicBlock* falseCl = createBasicBlock("UserCtp Not OK");
  PHINode* node = llvm::PHINode::Create(Ty, "", trueCl);
  node->addIncoming(res, currentBlock);
  BranchInst::Create(falseCl, trueCl, test, currentBlock);
  std::vector<Value*> Args;
  Args.push_back(ctpCache);
  Args.push_back(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), index));
  Args.push_back(GV);
  res = CallInst::Create(module->SpecialCtpLookupFunction, Args.begin(),
                         Args.end(), "", falseCl);
  node->addIncoming(res, falseCl);
  BranchInst::Create(trueCl, falseCl);
  currentBlock = trueCl;
  args.push_back(node);
#endif

  if (!meth) {
    // Make sure the class is loaded before materializing the method.
    uint32 clIndex = ctpInfo->getClassIndexFromMethod(index);
    UserCommonClass* cl = 0;
    Value* Cl = getResolvedCommonClass(clIndex, false, &cl);
    if (!cl) {
      CallInst::Create(module->ForceLoadedCheckFunction, Cl, "", currentBlock);
    }
    func =  TheCompiler->addCallback(compilingClass, index, signature, false);
  } else {
    func = TheCompiler->getMethod(meth);
  }

  if (meth && canBeInlined(meth)) {
    val = invokeInline(meth, args);
  } else {
    val = invoke(func, args, "", currentBlock);
  }
  
  const llvm::Type* retType = virtualType->getReturnType();
  if (retType != Type::getVoidTy(getGlobalContext())) {
    push(val, signature->getReturnType()->isUnsigned());
    if (retType == Type::getDoubleTy(getGlobalContext()) || retType == Type::getInt64Ty(getGlobalContext())) {
      push(module->constantZero, false);
    }
  }

}

void JavaJIT::invokeStatic(uint16 index) {
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* cl = 0;
  ctpInfo->nameOfStaticOrSpecialMethod(index, cl, name, signature);
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* staticType = LSI->getStaticType();
  llvm::Instruction* val = 0;
  
  std::vector<Value*> args; // size = [signature->nbIn + 2]; 
  FunctionType::param_iterator it  = staticType->param_end();
  makeArgs(it, index, args, signature->nbArguments);
  ctpInfo->markAsStaticCall(index);

  JnjvmBootstrapLoader* loader = compilingClass->classLoader->bootstrapLoader;
  if (cl->equals(loader->mathName)) {
    val = lowerMathOps(name, args);
  }

  else if (cl->equals(loader->stackWalkerName)) {
    callsStackWalker = true;
  }

  if (!val) {
    JavaMethod* meth = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_STATIC,
                                                            signature);
    
    Value* func = 0;
    if (meth) {
      func = TheCompiler->getMethod(meth);
    } else {
      func = TheCompiler->addCallback(compilingClass, index, signature, true);
    }

#if defined(ISOLATE_SHARING)
    Value* newCtpCache = getConstantPoolAt(index,
                                           module->StaticCtpLookupFunction,
                                           module->ConstantPoolType, 0,
                                           false);
    args.push_back(newCtpCache);
#endif
    
    uint32 clIndex = ctpInfo->getClassIndexFromMethod(index);
    UserClass* cl = 0;
    Value* Cl = getResolvedClass(clIndex, true, true, &cl);
    if (!meth || (cl && needsInitialisationCheck(cl, compilingClass))) {
      CallInst::Create(module->ForceInitialisationCheckFunction, Cl, "",
                       currentBlock);
    }

    if (meth && canBeInlined(meth)) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }

  }

  const llvm::Type* retType = staticType->getReturnType();
  if (retType != Type::getVoidTy(getGlobalContext())) {
    push(val, signature->getReturnType()->isUnsigned());
    if (retType == Type::getDoubleTy(getGlobalContext()) || retType == Type::getInt64Ty(getGlobalContext())) {
      push(module->constantZero, false);
    }
  }
}

Value* JavaJIT::getConstantPoolAt(uint32 index, Function* resolver,
                                  const Type* returnType,
                                  Value* additionalArg, bool doThrow) {

// This makes unswitch loop very unhappy time-wise, but makes GVN happy
// number-wise. IMO, it's better to have this than Unswitch.
#ifdef ISOLATE_SHARING
  Value* CTP = ctpCache;
  Value* Cl = GetElementPtrInst::Create(CTP, module->ConstantOne, "",
                                        currentBlock);
  Cl = new LoadInst(Cl, "", currentBlock);
#else
  JavaConstantPool* ctp = compilingClass->ctpInfo;
  Value* CTP = TheCompiler->getConstantPool(ctp);
  Value* Cl = TheCompiler->getNativeClass(compilingClass);
#endif

  std::vector<Value*> Args;
  Args.push_back(resolver);
  Args.push_back(CTP);
  Args.push_back(Cl);
  Args.push_back(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), index));
  if (additionalArg) Args.push_back(additionalArg);

  Value* res = 0;
  if (doThrow) {
    res = invoke(module->GetConstantPoolAtFunction, Args, "",
                 currentBlock);
  } else {
    res = CallInst::Create(module->GetConstantPoolAtFunction, Args.begin(),
                           Args.end(), "", currentBlock);
  }
  
  const Type* realType = 
    module->GetConstantPoolAtFunction->getReturnType();
  if (returnType == Type::getInt32Ty(getGlobalContext())) {
    return new PtrToIntInst(res, Type::getInt32Ty(getGlobalContext()), "", currentBlock);
  } else if (returnType != realType) {
    return new BitCastInst(res, returnType, "", currentBlock);
  } 
  
  return res;
}

Value* JavaJIT::getResolvedCommonClass(uint16 index, bool doThrow,
                                       UserCommonClass** alreadyResolved) {
    
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  CommonClass* cl = ctpInfo->getMethodClassIfLoaded(index);
  Value* node = 0;
  if (cl && (!cl->isClass() || cl->asClass()->isResolved())) {
    if (alreadyResolved) *alreadyResolved = cl;
    node = TheCompiler->getNativeClass(cl);
    // Since we only allocate for array classes that we own and
    // ony primitive arrays are already allocated, verify that the class
    // array is not external.
    if (TheCompiler->isStaticCompiling() && cl->isArray() && 
        node->getType() != module->JavaClassArrayType) {
      node = new LoadInst(node, "", currentBlock);
    }
    if (node->getType() != module->JavaCommonClassType) {
      node = new BitCastInst(node, module->JavaCommonClassType, "",
                             currentBlock);
    }
  } else {
    node = getConstantPoolAt(index, module->ClassLookupFunction,
                             module->JavaCommonClassType, 0, doThrow);
  }
  
  return node;
}

Value* JavaJIT::getResolvedClass(uint16 index, bool clinit, bool doThrow,
                                 Class** alreadyResolved) {
    
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  Class* cl = (Class*)(ctpInfo->getMethodClassIfLoaded(index));
  Value* node = 0;
  bool needsInit = true;
  if (cl && cl->isResolved()) {
    if (alreadyResolved) (*alreadyResolved) = cl;
    node = TheCompiler->getNativeClass(cl);
    needsInit = needsInitialisationCheck(cl, compilingClass);
  } else {
    node = getConstantPoolAt(index, module->ClassLookupFunction,
                             module->JavaClassType, 0, doThrow);
  }
 

  if (clinit && needsInit) {
    if (node->getType() != module->JavaClassType) {
      node = new BitCastInst(node, module->JavaClassType, "", currentBlock);
    }
    return invoke(module->InitialisationCheckFunction, node, "",
                  currentBlock);
  } else {
    return node;
  }
}

void JavaJIT::invokeNew(uint16 index) {
  
  Class* cl = 0;
  Value* Cl = getResolvedClass(index, true, true, &cl);
          
  Value* VT = 0;
  Value* Size = 0;
  
  if (cl) {
    VT = TheCompiler->getVirtualTable(cl->virtualVT);
    LLVMClassInfo* LCI = TheCompiler->getClassInfo(cl);
    Size = LCI->getVirtualSize();
    
    bool needsCheck = needsInitialisationCheck(cl, compilingClass);
    if (needsCheck) {
      Cl = invoke(module->ForceInitialisationCheckFunction, Cl, "",
                  currentBlock);
    }

  } else {
    VT = CallInst::Create(module->GetVTFromClassFunction, Cl, "",
                          currentBlock);
    Size = CallInst::Create(module->GetObjectSizeFromClassFunction, Cl,
                            "", currentBlock);
  }
 
  VT = new BitCastInst(VT, module->ptrType, "", currentBlock);
  Value* val = invoke(module->AllocateFunction, Size, VT, "",
                      currentBlock);
 
  val = new BitCastInst(val, module->JavaObjectType, "", currentBlock);
  push(val, false);
}

Value* JavaJIT::ldResolved(uint16 index, bool stat, Value* object, 
                           const Type* fieldType, const Type* fieldTypePtr) {
  JavaConstantPool* info = compilingClass->ctpInfo;
  
  JavaField* field = info->lookupField(index, stat);
  if (field && field->classDef->isResolved()) {
    LLVMClassInfo* LCI = TheCompiler->getClassInfo(field->classDef);
    LLVMFieldInfo* LFI = TheCompiler->getFieldInfo(field);
    const Type* type = 0;
    if (stat) {
      type = LCI->getStaticType();
      Value* Cl = TheCompiler->getNativeClass(field->classDef);
      bool needsCheck = needsInitialisationCheck(field->classDef,
                                                 compilingClass);
      if (needsCheck) {
        Cl = invoke(module->InitialisationCheckFunction, Cl, "",
                    currentBlock);
      }
#if !defined(ISOLATE) && !defined(ISOLATE_SHARING)
      if (needsCheck) {
        CallInst::Create(module->ForceInitialisationCheckFunction, Cl, "",
                         currentBlock);
      }

      object = TheCompiler->getStaticInstance(field->classDef);
#else
      object = CallInst::Create(module->GetStaticInstanceFunction, Cl, "",
                                currentBlock); 
#endif
    } else {
      type = LCI->getVirtualType();
    }
    
    Value* objectConvert = new BitCastInst(object, type, "", currentBlock);

    Value* args[2] = { module->constantZero, LFI->getOffset() };
    Value* ptr = llvm::GetElementPtrInst::Create(objectConvert,
                                                 args, args + 2, "",
                                                 currentBlock);
    return ptr;
  }

  const Type* Pty = module->arrayPtrType;
  Constant* zero = module->constantZero;
    
  Function* func = stat ? module->StaticFieldLookupFunction :
                          module->VirtualFieldLookupFunction;
    
  const Type* returnType = 0;
  if (stat)
    returnType = module->ptrType;
  else
    returnType = Type::getInt32Ty(getGlobalContext());

  Value* ptr = getConstantPoolAt(index, func, returnType, 0, true);
  if (!stat) {
    Value* tmp = new BitCastInst(object, Pty, "", currentBlock);
    Value* args[2] = { zero, ptr };
    ptr = GetElementPtrInst::Create(tmp, args, args + 2, "", currentBlock);
  }
    
  return new BitCastInst(ptr, fieldTypePtr, "", currentBlock);
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
  bool unsign = topIsUnsigned();
  Value* val = pop(); 
  
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  
  if (type == Type::getInt64Ty(getGlobalContext()) || type == Type::getDoubleTy(getGlobalContext())) {
    val = pop();
  }
  
  Value* ptr = ldResolved(index, true, 0, type, LAI.llvmTypePtr);
  
  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, unsign);
  }
  
  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getStaticField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  
  Value* ptr = ldResolved(index, true, 0, type, LAI.llvmTypePtr);
  
  bool final = false;
#if !defined(ISOLATE) && !defined(ISOLATE_SHARING)
  JnjvmBootstrapLoader* JBL = compilingClass->classLoader->bootstrapLoader;
  if (!compilingMethod->name->equals(JBL->clinitName)) {
    JavaField* field = compilingClass->ctpInfo->lookupField(index, true);
    if (field && field->classDef->isReady()) final = isFinal(field->access);
    if (final) {
      void* Obj = field->classDef->getStaticInstance();
      if (sign->isPrimitive()) {
        const PrimitiveTypedef* prim = (PrimitiveTypedef*)sign;
        if (prim->isInt()) {
          sint32 val = field->getInt32Field(Obj);
          push(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), val), false);
        } else if (prim->isByte()) {
          sint8 val = (sint8)field->getInt8Field(Obj);
          push(ConstantInt::get(Type::getInt8Ty(getGlobalContext()), val), false);
        } else if (prim->isBool()) {
          uint8 val = (uint8)field->getInt8Field(Obj);
          push(ConstantInt::get(Type::getInt8Ty(getGlobalContext()), val), true);
        } else if (prim->isShort()) {
          sint16 val = (sint16)field->getInt16Field(Obj);
          push(ConstantInt::get(Type::getInt16Ty(getGlobalContext()), val), false);
        } else if (prim->isChar()) {
          uint16 val = (uint16)field->getInt16Field(Obj);
          push(ConstantInt::get(Type::getInt16Ty(getGlobalContext()), val), true);
        } else if (prim->isLong()) {
          sint64 val = (sint64)field->getLongField(Obj);
          push(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), val), false);
        } else if (prim->isFloat()) {
          float val = (float)field->getFloatField(Obj);
          push(ConstantFP::get(Type::getFloatTy(getGlobalContext()), val), false);
        } else if (prim->isDouble()) {
          double val = (double)field->getDoubleField(Obj);
          push(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), val), false);
        } else {
          abort();
        }
      } else {
        if (TheCompiler->isStaticCompiling()) {
          JavaObject* val = field->getObjectField(Obj);
          Value* V = TheCompiler->getFinalObject(val);
          push(V, false);
        } else {
          Value* V = CallInst::Create(module->GetFinalObjectFieldFunction, ptr,
                                      "", currentBlock);
          
          push(V, false);
        } 
      }
    }
  }
#endif

  if (!final) push(new LoadInst(ptr, "", currentBlock), sign->isUnsigned());
  if (type == Type::getInt64Ty(getGlobalContext()) || type == Type::getDoubleTy(getGlobalContext())) {
    push(module->constantZero, false);
  }
}

void JavaJIT::setVirtualField(uint16 index) {
  bool unsign = topIsUnsigned();
  Value* val = pop();
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  
  if (type == Type::getInt64Ty(getGlobalContext()) || type == Type::getDoubleTy(getGlobalContext())) {
    val = pop();
  }
  
  Value* object = pop();
  JITVerifyNull(object);
  Value* ptr = ldResolved(index, false, object, type, LAI.llvmTypePtr);

  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, unsign);
  }

  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getVirtualField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  Value* obj = pop();
  JITVerifyNull(obj);
  
  Value* ptr = ldResolved(index, false, obj, type, LAI.llvmTypePtr);
  
  JnjvmBootstrapLoader* JBL = compilingClass->classLoader->bootstrapLoader;
  bool final = false;
  if (!compilingMethod->name->equals(JBL->initName)) {
    JavaField* field = compilingClass->ctpInfo->lookupField(index, false);
    if (field) final = isFinal(field->access);
    if (final) {
      Function* F = 0;
      if (sign->isPrimitive()) {
        const PrimitiveTypedef* prim = (PrimitiveTypedef*)sign;
        if (prim->isInt()) {
          F = module->GetFinalInt32FieldFunction;
        } else if (prim->isByte()) {
          F = module->GetFinalInt8FieldFunction;
        } else if (prim->isBool()) {
          F = module->GetFinalInt8FieldFunction;
        } else if (prim->isShort()) {
          F = module->GetFinalInt16FieldFunction;
        } else if (prim->isChar()) {
          F = module->GetFinalInt16FieldFunction;
        } else if (prim->isLong()) {
          F = module->GetFinalLongFieldFunction;
        } else if (prim->isFloat()) {
          F = module->GetFinalFloatFieldFunction;
        } else if (prim->isDouble()) {
          F = module->GetFinalDoubleFieldFunction;
        } else {
          abort();
        }
      } else {
        F = module->GetFinalObjectFieldFunction;
      }
      push(CallInst::Create(F, ptr, "", currentBlock), sign->isUnsigned());
    }
  }
 
  if (!final) push(new LoadInst(ptr, "", currentBlock), sign->isUnsigned());
  if (type == Type::getInt64Ty(getGlobalContext()) || type == Type::getDoubleTy(getGlobalContext())) {
    push(module->constantZero, false);
  }
}


void JavaJIT::invokeInterface(uint16 index, bool buggyVirtual) {
  
  // Do the usual
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  const UTF8* name = 0;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index, name);
  
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  const llvm::PointerType* virtualPtrType = LSI->getVirtualPtrType();

  std::vector<Value*> args; // size = [signature->nbIn + 3];

  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbArguments + 1);
  
  const llvm::Type* retType = virtualType->getReturnType();
  BasicBlock* endBlock = createBasicBlock("end virtual invoke");
  PHINode * node = 0;
  if (retType != Type::getVoidTy(getGlobalContext())) {
    node = PHINode::Create(retType, "", endBlock);
  }
  
  JITVerifyNull(args[0]);
  
  Value* zero = module->constantZero;
  Value* one = module->constantOne;

#ifndef ISOLATE_SHARING
  // ok now the cache
  Enveloppe& enveloppe = buggyVirtual ?
    *(new (compilingClass->classLoader->allocator, "Enveloppe") Enveloppe()) :
    compilingMethod->enveloppes[nbEnveloppes++];
  if (!inlining)
    enveloppe.initialise(compilingClass, name, signature->keyName);
   
  Value* llvmEnv = TheCompiler->getEnveloppe(&enveloppe);
#else
  Value* llvmEnv = getConstantPoolAt(index,
                                     module->EnveloppeLookupFunction,
                                     module->EnveloppeType, 0, false);
#endif

  Value* args1[2] = { zero, zero };
  Value* cachePtr = GetElementPtrInst::Create(llvmEnv, args1, args1 + 2,
                                              "", currentBlock);
  Value* cache = new LoadInst(cachePtr, "", currentBlock);

  Value* VT = CallInst::Create(module->GetVTFunction, args[0], "",
                               currentBlock);
  Value* args3[2] = { zero, one };
  Value* lastCiblePtr = GetElementPtrInst::Create(cache, args3, args3 + 2, "",
                                                  currentBlock);
  Value* lastCible = new LoadInst(lastCiblePtr, "", currentBlock);

  Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, VT, lastCible,
                            "");
  
  BasicBlock* ifTrue = createBasicBlock("cache ok");
  BasicBlock* ifFalse = createBasicBlock("cache not ok");
  BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
  
  currentBlock = ifFalse;
  Value* _meth = invoke(module->InterfaceLookupFunction, cache, args[0],
                        "", ifFalse);
  Value* meth = new BitCastInst(_meth, virtualPtrType, "", 
                                currentBlock);
#ifdef ISOLATE_SHARING
  Value* cache2 = new LoadInst(cachePtr, "", currentBlock);
  Value* newCtpCache = CallInst::Create(module->GetCtpCacheNodeFunction,
                                        cache2, "", currentBlock);
  args.push_back(newCtpCache);
#endif
  Value* ret = invoke(meth, args, "", currentBlock);
  if (node) {
    node->addIncoming(ret, currentBlock);
  }
  BranchInst::Create(endBlock, currentBlock);

  currentBlock = ifTrue;

  Value* methPtr = GetElementPtrInst::Create(cache, args1, args1 + 2,
                                             "", currentBlock);

  _meth = new LoadInst(methPtr, "", currentBlock);
  meth = new BitCastInst(_meth, virtualPtrType, "", currentBlock);
  
#ifdef ISOLATE_SHARING
  args.pop_back();
  cache = new LoadInst(cachePtr, "", currentBlock);
  newCtpCache = CallInst::Create(module->GetCtpCacheNodeFunction,
                                 cache, "", currentBlock);
  args.push_back(newCtpCache);
#endif
  ret = invoke(meth, args, "", currentBlock);
  BranchInst::Create(endBlock, currentBlock);

  if (node) {
    node->addIncoming(ret, currentBlock);
  }

  currentBlock = endBlock;
  if (node) {
    push(node, signature->getReturnType()->isUnsigned());
    if (retType == Type::getDoubleTy(getGlobalContext()) ||
        retType == Type::getInt64Ty(getGlobalContext())) {
      push(module->constantZero, false);
    }
  }
}


#ifdef DWARF_EXCEPTIONS
#include "ExceptionsDwarf.inc"
#else
#include "ExceptionsCheck.inc"
#endif
