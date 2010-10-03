//===----------- JavaJIT.cpp - Java just in time compiler -----------------===//
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
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Support/CFG.h>

#include "mvm/JIT.h"

#include "debug.h"
#include "JavaArray.h"
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

#include "j3/JavaLLVMCompiler.h"
#include "j3/J3Intrinsics.h"

using namespace j3;
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
  bool canBeDirect = false;
  Value* val = NULL;  // The return from the method.
 
  if ((cl && isFinal(cl->access)) || 
      (meth && (isFinal(meth->access) || isPrivate(meth->access)))) {
    canBeDirect = true;
  }

  if (meth && isInterface(meth->classDef->access)) {
    // This can happen because we compute miranda methods before resolving
    // interfaces.
		return invokeInterface(index);
	}
 
  const UTF8* name = 0;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index, name);
 
  if (TheCompiler->isStaticCompiling()) {
    Value* obj = objectStack[stack.size() - signature->nbArguments - 1];
    JavaObject* source = TheCompiler->getFinalObject(obj);
    if (source) {
      canBeDirect = true;
      CommonClass* sourceClass = JavaObject::getClass(source);
      Class* lookup = sourceClass->isArray() ? sourceClass->super :
                                               sourceClass->asClass();
      meth = lookup->lookupMethodDontThrow(name, signature->keyName, false,
                                           true, 0);
    }
    CommonClass* unique = TheCompiler->getUniqueBaseClass(cl);
    if (unique) {
      canBeDirect = true;
      Class* lookup = unique->isArray() ? unique->super : unique->asClass();
      meth = lookup->lookupMethodDontThrow(name, signature->keyName, false,
                                           true, 0);
    }
  }
 
  Typedef* retTypedef = signature->getReturnType();
  std::vector<Value*> args; // size = [signature->nbIn + 3];
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  FunctionType::param_iterator it  = virtualType->param_end();
  const llvm::Type* retType = virtualType->getReturnType();

  bool needsInit = false;
  if (canBeDirect && meth && !TheCompiler->needsCallback(meth, &needsInit)) {
    makeArgs(it, index, args, signature->nbArguments + 1);
    JITVerifyNull(args[0]);
    val = invoke(TheCompiler->getMethod(meth), args, "", currentBlock);
  } else {

    BasicBlock* endBlock = 0;
    PHINode* node = 0;
#if 0
    // TODO: enable this only when inlining?
    if (meth && !isAbstract(meth->access)) {
      Value* cl = CallInst::Create(intrinsics->GetClassFunction, args[0], "",
                                   currentBlock);
      Value* cl2 = intrinsics->getNativeClass(meth->classDef);
      if (cl2->getType() != intrinsics->JavaCommonClassType) {
        cl2 = new BitCastInst(cl2, intrinsics->JavaCommonClassType, "", currentBlock);
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
        Function* func = intrinsics->getMethod(meth);
        res = invoke(func, args, "", currentBlock);
      }
      BranchInst::Create(endBlock, currentBlock);
      if (retType != Type::getVoidTy(*llvmContext)) {
        node = PHINode::Create(virtualType->getReturnType(), "", endBlock);
        node->addIncoming(res, currentBlock);
      }
      currentBlock = falseBlock;
    }
#endif

    Value* indexes2[2];
    indexes2[0] = intrinsics->constantZero;

#ifdef ISOLATE_SHARING
    Value* indexesCtp; //[3];
#endif
    if (meth) {
      LLVMMethodInfo* LMI = TheCompiler->getMethodInfo(meth);
      Constant* Offset = LMI->getOffset();
      indexes2[1] = Offset;
#ifdef ISOLATE_SHARING
      indexesCtp = ConstantInt::get(Type::getInt32Ty(*llvmContext),
                                    Offset->getZExtValue() * -1);
#endif
    } else {
   
      GlobalVariable* GV = new GlobalVariable(*llvmFunction->getParent(),
                                              Type::getInt32Ty(*llvmContext),
                                              false,
                                              GlobalValue::ExternalLinkage,
                                              intrinsics->constantZero, "");
    
      BasicBlock* resolveVirtual = createBasicBlock("resolveVirtual");
      BasicBlock* endResolveVirtual = createBasicBlock("endResolveVirtual");
      PHINode* node = PHINode::Create(Type::getInt32Ty(*llvmContext), "",
                                      endResolveVirtual);

      Value* load = new LoadInst(GV, "", false, currentBlock);
      Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, load,
                                 intrinsics->constantZero, "");
      BranchInst::Create(resolveVirtual, endResolveVirtual, test, currentBlock);
      node->addIncoming(load, currentBlock);
      currentBlock = resolveVirtual;
      std::vector<Value*> Args;
      Args.push_back(TheCompiler->getNativeClass(compilingClass));
      Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), index));
      Args.push_back(GV);
      Value* targetObject = getTarget(virtualType->param_end(),
                                      signature->nbArguments + 1);
      Args.push_back(new LoadInst(
          targetObject, "", TheCompiler->useCooperativeGC(), currentBlock));
      load = invoke(intrinsics->VirtualLookupFunction, Args, "", currentBlock);
      node->addIncoming(load, currentBlock);
      BranchInst::Create(endResolveVirtual, currentBlock);
      currentBlock = endResolveVirtual;

      indexes2[1] = node;
#ifdef ISOLATE_SHARING
      Value* mul = BinaryOperator::CreateMul(val, intrinsics->constantMinusOne,
                                             "", currentBlock);
      indexesCtp = mul;
#endif
    }

    makeArgs(it, index, args, signature->nbArguments + 1);
    JITVerifyNull(args[0]);
    Value* VT = CallInst::Create(intrinsics->GetVTFunction, args[0], "",
                                 currentBlock);
 
    Value* FuncPtr = GetElementPtrInst::Create(VT, indexes2, indexes2 + 2, "",
                                               currentBlock);
    
    Value* Func = new LoadInst(FuncPtr, "", currentBlock);
  
    Func = new BitCastInst(Func, LSI->getVirtualPtrType(), "", currentBlock);
#ifdef ISOLATE_SHARING
    Value* CTP = GetElementPtrInst::Create(VT, indexesCtp, "", currentBlock);
    
    CTP = new LoadInst(CTP, "", currentBlock);
    CTP = new BitCastInst(CTP, intrinsics->ConstantPoolType, "", currentBlock);
    args.push_back(CTP);
#endif
    val = invoke(Func, args, "", currentBlock);
  
    if (endBlock) {
      if (node) {
        node->addIncoming(val, currentBlock);
        val = node;
      }
      BranchInst::Create(endBlock, currentBlock);
      currentBlock = endBlock;
    }
  }

  if (retType != Type::getVoidTy(*llvmContext)) {
    if (retType == intrinsics->JavaObjectType) {
      JnjvmClassLoader* JCL = compilingClass->classLoader;
      push(val, false, signature->getReturnType()->findAssocClass(JCL));
    } else {
      push(val, retTypedef->isUnsigned());
      if (retType == Type::getDoubleTy(*llvmContext) || retType == Type::getInt64Ty(*llvmContext)) {
        push(intrinsics->constantZero, false);
      }
    }
  }
}

llvm::Value* JavaJIT::getCurrentThread(const llvm::Type* Ty) {
  Value* FrameAddr = CallInst::Create(intrinsics->llvm_frameaddress,
                                     	intrinsics->constantZero, "", currentBlock);
  Value* threadId = new PtrToIntInst(FrameAddr, intrinsics->pointerSizeType, "",
                              			 currentBlock);
  threadId = BinaryOperator::CreateAnd(threadId, intrinsics->constantThreadIDMask,
                                       "", currentBlock);
  threadId = new IntToPtrInst(threadId, Ty, "", currentBlock);

  return threadId;
}

extern "C" void j3ThrowExceptionFromJIT();

llvm::Function* JavaJIT::nativeCompile(intptr_t natPtr) {
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "native compile %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
  bool stat = isStatic(compilingMethod->access);

  const FunctionType *funcType = llvmFunction->getFunctionType();
  const llvm::Type* returnType = funcType->getReturnType();
  
  bool j3 = false;
  
  const UTF8* jniConsClName = compilingClass->name;
  const UTF8* jniConsName = compilingMethod->name;
  const UTF8* jniConsType = compilingMethod->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  char* functionName = (char*)alloca(3 + JNI_NAME_PRE_LEN + 
                            ((mnlen + clen + mtlen) << 3));
  
  if (!natPtr)
    natPtr = compilingClass->classLoader->nativeLookup(compilingMethod, j3,
                                                       functionName);
  
  if (!natPtr && !TheCompiler->isStaticCompiling()) {
    currentBlock = createBasicBlock("start");
    CallInst::Create(intrinsics->ThrowExceptionFromJITFunction, "", currentBlock);
    if (returnType != Type::getVoidTy(*llvmContext))
      ReturnInst::Create(*llvmContext, Constant::getNullValue(returnType), currentBlock);
    else
      ReturnInst::Create(*llvmContext, currentBlock);
  
    PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "end native compile %s.%s\n",
                UTF8Buffer(compilingClass->name).cString(),
                UTF8Buffer(compilingMethod->name).cString());
  
    return llvmFunction;
  }
  
  
  Function* func = llvmFunction;
  if (j3) {
    compilingMethod->setCompiledPtr((void*)natPtr, functionName);
    llvmFunction->clearGC();
    return llvmFunction;
  }


  currentExceptionBlock = endExceptionBlock = 0;
  currentBlock = createBasicBlock("start");
  endBlock = createBasicBlock("end block");
  
  if (returnType != Type::getVoidTy(*llvmContext)) {
    endNode = PHINode::Create(returnType, "", endBlock);
  }
  
  // Allocate currentLocalIndexNumber pointer
  Value* temp = new AllocaInst(Type::getInt32Ty(*llvmContext), "",
                               currentBlock);
  new StoreInst(intrinsics->constantZero, temp, false, currentBlock);
  
  // Allocate oldCurrentLocalIndexNumber pointer
  Value* oldCLIN = new AllocaInst(PointerType::getUnqual(Type::getInt32Ty(*llvmContext)), "",
                                  currentBlock);
  
  Constant* sizeF = ConstantInt::get(Type::getInt32Ty(*llvmContext), 2 * sizeof(void*));
  Value* Frame = new AllocaInst(Type::getInt8Ty(*llvmContext), sizeF, "", currentBlock);
  
  uint32 nargs = func->arg_size() + 1 + (stat ? 1 : 0); 
  std::vector<Value*> nativeArgs;
  
  
  Value* threadId = getCurrentThread(intrinsics->JavaThreadType);

  Value* geps[2] = { intrinsics->constantZero,
                     intrinsics->OffsetJNIInThreadConstant };

  Value* jniEnv = GetElementPtrInst::Create(threadId, geps, geps + 2, "",
                                            currentBlock);
 
  jniEnv = new BitCastInst(jniEnv, intrinsics->ptrType, "", currentBlock);

  nativeArgs.push_back(jniEnv);

  uint32 index = 0;
  if (stat) {
#ifdef ISOLATE_SHARING
    Value* val = getClassCtp();
    Value* cl = CallInst::Create(intrinsics->GetClassDelegateePtrFunction,
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
    
    if (i->getType() == intrinsics->JavaObjectType) {
      BasicBlock* BB = createBasicBlock("");
      BasicBlock* NotZero = createBasicBlock("");
      const Type* Ty = PointerType::getUnqual(intrinsics->JavaObjectType);
      PHINode* node = PHINode::Create(Ty, "", BB);

      Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, i,
                                 intrinsics->JavaObjectNullConstant, "");

      node->addIncoming(Constant::getNullValue(Ty), currentBlock);
      BranchInst::Create(BB, NotZero, test, currentBlock);

      currentBlock = NotZero;

      Instruction* temp = new AllocaInst(intrinsics->JavaObjectType, "",
                                         func->begin()->getTerminator());
      if (i == func->arg_begin() && !stat) {
        this->thisObject = temp;
      }
      
      if (TheCompiler->useCooperativeGC()) {
        Value* GCArgs[2] = { 
          new BitCastInst(temp, intrinsics->ptrPtrType, "",
                          func->begin()->getTerminator()),
          intrinsics->constantPtrNull
        };
        
        CallInst::Create(intrinsics->llvm_gc_gcroot, GCArgs, GCArgs + 2, "",
                         func->begin()->getTerminator());
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
  if (returnType == intrinsics->JavaObjectType) {
    ResultObject = new AllocaInst(intrinsics->JavaObjectType, "",
                                  func->begin()->begin());
    
    if (TheCompiler->useCooperativeGC()) {
      
      Value* GCArgs[2] = { 
        new BitCastInst(ResultObject, intrinsics->ptrPtrType, "", currentBlock),
        intrinsics->constantPtrNull
      };
      
      CallInst::Create(intrinsics->llvm_gc_gcroot, GCArgs, GCArgs + 2, "",
                       currentBlock);
    } else {
      new StoreInst(intrinsics->JavaObjectNullConstant, ResultObject, "",
                    currentBlock);
    }
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

  // Synchronize before saying we're entering native
  if (isSynchro(compilingMethod->access))
    beginSynchronize();
  
  Value* Args4[3] = { temp, oldCLIN, Frame };

  CallInst::Create(intrinsics->StartJNIFunction, Args4, Args4 + 3, "",
                   currentBlock);
  
  Value* result = llvm::CallInst::Create(nativeFunc, nativeArgs.begin(),
                                         nativeArgs.end(), "", currentBlock);

  if (returnType == intrinsics->JavaObjectType) {
    const Type* Ty = PointerType::getUnqual(intrinsics->JavaObjectType);
    Constant* C = Constant::getNullValue(Ty);
    Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, result, C, "");
    BasicBlock* loadBlock = createBasicBlock("");

    endNode->addIncoming(intrinsics->JavaObjectNullConstant, currentBlock);
    BranchInst::Create(endBlock, loadBlock, cmp, currentBlock);

    currentBlock = loadBlock;
    result = new LoadInst(
        result, "", TheCompiler->useCooperativeGC(), currentBlock);
    new StoreInst(result, ResultObject, "", currentBlock);
    endNode->addIncoming(result, currentBlock);

  } else if (returnType != Type::getVoidTy(*llvmContext)) {
    endNode->addIncoming(result, currentBlock);
  }
  
  BranchInst::Create(endBlock, currentBlock);


  currentBlock = endBlock; 
 
  Value* Args2[1] = { oldCLIN };

  CallInst::Create(intrinsics->EndJNIFunction, Args2, Args2 + 1, "", currentBlock);
  
  // Synchronize after leaving native.
  if (isSynchro(compilingMethod->access))
    endSynchronize();
  
  if (returnType != Type::getVoidTy(*llvmContext))
    ReturnInst::Create(*llvmContext, endNode, currentBlock);
  else
    ReturnInst::Create(*llvmContext, currentBlock);
  
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "end native compile %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
  if (codeInfo.size()) { 
    compilingMethod->codeInfo = new CodeLineInfo[codeInfo.size()];
    for (uint32 i = 0; i < codeInfo.size(); i++) {
      compilingMethod->codeInfo[i].lineNumber = codeInfo[i].lineNumber;
      compilingMethod->codeInfo[i].ctpIndex = codeInfo[i].ctpIndex;
      compilingMethod->codeInfo[i].bytecodeIndex = codeInfo[i].bytecodeIndex;
      compilingMethod->codeInfo[i].bytecode = codeInfo[i].bytecode;
    }
  } else {
    compilingMethod->codeInfo = NULL;
  }
 
  return llvmFunction;
}

void JavaJIT::monitorEnter(Value* obj) {
  std::vector<Value*> gep;
  gep.push_back(intrinsics->constantZero);
  gep.push_back(intrinsics->JavaObjectLockOffsetConstant);
  Value* lockPtr = GetElementPtrInst::Create(obj, gep.begin(), gep.end(), "",
                                             currentBlock);
  
  Value* lock = new LoadInst(lockPtr, "", currentBlock);
  lock = new PtrToIntInst(lock, intrinsics->pointerSizeType, "", currentBlock);
  Value* NonLockBitsMask = ConstantInt::get(intrinsics->pointerSizeType,
                                            mvm::NonLockBitsMask);

  lock = BinaryOperator::CreateAnd(lock, NonLockBitsMask, "", currentBlock);

  lockPtr = new BitCastInst(lockPtr, 
                            PointerType::getUnqual(intrinsics->pointerSizeType),
                            "", currentBlock);
  Value* threadId = getCurrentThread(intrinsics->MutatorThreadType);
  threadId = new PtrToIntInst(threadId, intrinsics->pointerSizeType, "",
                              currentBlock);
  Value* newValMask = BinaryOperator::CreateOr(threadId, lock, "",
                                               currentBlock);

  std::vector<Value*> atomicArgs;
  atomicArgs.push_back(lockPtr);
  atomicArgs.push_back(lock);
  atomicArgs.push_back(newValMask);

  // Do the atomic compare and swap.
  Value* atomic = CallInst::Create(intrinsics->llvm_atomic_lcs_ptr,
                                   atomicArgs.begin(), atomicArgs.end(), "",
                                   currentBlock);
  
  Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, atomic,
                            lock, "");
  
  BasicBlock* OK = createBasicBlock("synchronize passed");
  BasicBlock* NotOK = createBasicBlock("synchronize did not pass");

  BranchInst::Create(OK, NotOK, cmp, currentBlock);

  // The atomic cas did not work.
  currentBlock = NotOK;
  CallInst::Create(intrinsics->AquireObjectFunction, obj, "", currentBlock);
  BranchInst::Create(OK, currentBlock);

  currentBlock = OK;
}

void JavaJIT::monitorExit(Value* obj) {
  std::vector<Value*> gep;
  gep.push_back(intrinsics->constantZero);
  gep.push_back(intrinsics->JavaObjectLockOffsetConstant);
  Value* lockPtr = GetElementPtrInst::Create(obj, gep.begin(), gep.end(), "",
                                             currentBlock);
  lockPtr = new BitCastInst(lockPtr, 
                            PointerType::getUnqual(intrinsics->pointerSizeType),
                            "", currentBlock);
  Value* lock = new LoadInst(lockPtr, "", currentBlock);
  Value* NonLockBitsMask = ConstantInt::get(
      intrinsics->pointerSizeType, mvm::NonLockBitsMask);

  Value* lockedMask = BinaryOperator::CreateAnd(
      lock, NonLockBitsMask, "", currentBlock);
  
  Value* threadId = getCurrentThread(intrinsics->MutatorThreadType);
  threadId = new PtrToIntInst(threadId, intrinsics->pointerSizeType, "",
                              currentBlock);
  
  Value* oldValMask = BinaryOperator::CreateOr(threadId, lockedMask, "",
                                               currentBlock);

  std::vector<Value*> atomicArgs;
  atomicArgs.push_back(lockPtr);
  atomicArgs.push_back(oldValMask);
  atomicArgs.push_back(lockedMask);

  // Do the atomic compare and swap.
  Value* atomic = CallInst::Create(intrinsics->llvm_atomic_lcs_ptr,
                                   atomicArgs.begin(), atomicArgs.end(), "",
                                   currentBlock);
  
  Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, atomic,
                            oldValMask, "");
  
  BasicBlock* OK = createBasicBlock("unsynchronize passed");
  BasicBlock* NotOK = createBasicBlock("unsynchronize did not pass");

  BranchInst::Create(OK, NotOK, cmp, currentBlock);

  // The atomic cas did not work.
  currentBlock = NotOK;
  CallInst::Create(intrinsics->ReleaseObjectFunction, obj, "", currentBlock);
  BranchInst::Create(OK, currentBlock);

  currentBlock = OK;
}

#ifdef ISOLATE_SHARING
Value* JavaJIT::getStaticInstanceCtp() {
  Value* cl = getClassCtp();
  Value* indexes[2] = { intrinsics->constantZero, module->constantSeven };
  Value* arg1 = GetElementPtrInst::Create(cl, indexes, indexes + 2,
                                          "", currentBlock);
  arg1 = new LoadInst(arg1, "", false, currentBlock);
  return arg1;
  
}

Value* JavaJIT::getClassCtp() {
  Value* indexes = intrinsics->constantOne;
  Value* arg1 = GetElementPtrInst::Create(ctpCache, indexes.begin(),
                                          indexes.end(),  "", currentBlock);
  arg1 = new LoadInst(arg1, "", false, currentBlock);
  arg1 = new BitCastInst(arg1, intrinsics->JavaClassType, "", currentBlock);
  return arg1;
}
#endif

void JavaJIT::beginSynchronize() {
  Value* obj = 0;
  if (isVirtual(compilingMethod->access)) {
    assert(thisObject != NULL && "beginSynchronize without this");
    obj = new LoadInst(
        thisObject, "", TheCompiler->useCooperativeGC(), currentBlock);
  } else {
    obj = TheCompiler->getJavaClassPtr(compilingClass);
    obj = new LoadInst(obj, "", false, currentBlock);
  }
  monitorEnter(obj);
}

void JavaJIT::endSynchronize() {
  Value* obj = 0;
  if (isVirtual(compilingMethod->access)) {
    assert(thisObject != NULL && "endSynchronize without this");
    obj = new LoadInst(
        thisObject, "", TheCompiler->useCooperativeGC(), currentBlock);
  } else {
    obj = TheCompiler->getJavaClassPtr(compilingClass);
    obj = new LoadInst(obj, "", false, currentBlock);
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
    } else if (uses == 1 && dyn_cast<StoreInst>(*(temp->use_begin()))) {
      dyn_cast<StoreInst>(*(temp->use_begin()))->eraseFromParent();
      temp->eraseFromParent();
    }
  }
}
  
static void removeUnusedObjects(std::vector<AllocaInst*>& objects,
                                J3Intrinsics* intrinsics, bool coop) {
  for (std::vector<AllocaInst*>::iterator i = objects.begin(),
       e = objects.end(); i != e; ++i) {
    AllocaInst* temp = *i;
    unsigned uses = temp->getNumUses();
    if (!uses) {
      temp->eraseFromParent();
    } else if (uses == 1 && dyn_cast<StoreInst>(*(temp->use_begin()))) {
      dyn_cast<StoreInst>(*(temp->use_begin()))->eraseFromParent();
      temp->eraseFromParent();
    } else {
      if (coop) {
        Instruction* I = new BitCastInst(temp, intrinsics->ptrPtrType, "");
        I->insertAfter(temp);
        Value* GCArgs[2] = { I, intrinsics->constantPtrNull };
        Instruction* C = CallInst::Create(intrinsics->llvm_gc_gcroot, GCArgs,
                                          GCArgs + 2, "");
        C->insertAfter(I);
      }
    }
  }
}

Instruction* JavaJIT::inlineCompile(BasicBlock*& curBB,
                                    BasicBlock* endExBlock,
                                    std::vector<Value*>& args) {
  DbgSubprogram = TheCompiler->GetDbgSubprogram(compilingMethod);

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

  opcodeInfos = new Opinfo[codeLen];
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExBlock;
  }
  
  BasicBlock* firstBB = llvmFunction->begin();
  
  if (firstBB->begin() != firstBB->end()) {
    Instruction* firstInstruction = firstBB->begin();

    for (int i = 0; i < maxLocals; i++) {
      intLocals.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, firstInstruction);
      doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "",
                                            firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, firstInstruction);
      longLocals.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, firstInstruction);
      floatLocals.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", firstInstruction));
      new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, firstInstruction);
      objectLocals.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                          firstInstruction));
     
      // The GCStrategy will already initialize the value.
      if (!TheCompiler->useCooperativeGC())
        new StoreInst(Constant::getNullValue(intrinsics->JavaObjectType), objectLocals.back(), false, firstInstruction);
    }
    for (int i = 0; i < maxStack; i++) {
      objectStack.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                           firstInstruction));
      addHighLevelType(objectStack.back(), upcalls->OfObject);
      intStack.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", firstInstruction));
      doubleStack.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "",
                                           firstInstruction));
      longStack.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", firstInstruction));
      floatStack.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", firstInstruction));
    }

  } else {
    for (int i = 0; i < maxLocals; i++) {
      intLocals.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, firstBB);
      doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, firstBB);
      longLocals.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, firstBB);
      floatLocals.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", firstBB));
      new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, firstBB);
      objectLocals.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                            firstBB));
      // The GCStrategy will already initialize the value.
      if (!TheCompiler->useCooperativeGC())
        new StoreInst(Constant::getNullValue(intrinsics->JavaObjectType), objectLocals.back(), false, firstBB);
    }
    
    for (int i = 0; i < maxStack; i++) {
      objectStack.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                           firstBB));
      addHighLevelType(objectStack.back(), upcalls->OfObject);
      intStack.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", firstBB));
      doubleStack.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "", firstBB));
      longStack.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", firstBB));
      floatStack.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", firstBB));
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
    Instruction* V = new StoreInst(*i, objectLocals[0], false, currentBlock);
    addHighLevelType(V, compilingClass);
    ++i;
    ++index;
    ++count;
    thisObject = objectLocals[0];
  }

  
  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const Typedef* cur = arguments[type];
    const Type* curType = (*i)->getType();

    if (curType == Type::getInt64Ty(*llvmContext)){
      new StoreInst(*i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur->isUnsigned()) {
      new StoreInst(new ZExtInst(*i, Type::getInt32Ty(*llvmContext), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt8Ty(*llvmContext) || curType == Type::getInt16Ty(*llvmContext)) {
      new StoreInst(new SExtInst(*i, Type::getInt32Ty(*llvmContext), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt32Ty(*llvmContext)) {
      new StoreInst(*i, intLocals[index], false, currentBlock);
    } else if (curType == Type::getDoubleTy(*llvmContext)) {
      new StoreInst(*i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (curType == Type::getFloatTy(*llvmContext)) {
      new StoreInst(*i, floatLocals[index], false, currentBlock);
    } else {
      Instruction* V = new StoreInst(*i, objectLocals[index], false, currentBlock);
      addHighLevelType(V, cur->findAssocClass(compilingClass->classLoader));
    }
  }
  
  readExceptionTable(reader, codeLen);
  
  // Lookup line number table attribute.
  uint16 nba = reader.readU2();
  for (uint16 i = 0; i < nba; ++i) {
    const UTF8* attName = compilingClass->ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    if (attName->equals(Attribut::lineNumberTableAttribut)) {
      uint16 lineLength = reader.readU2();
      for (uint16 i = 0; i < lineLength; ++i) {
        uint16 pc = reader.readU2();
        uint16 ln = reader.readU2();
        opcodeInfos[pc].lineNumber = ln;
      }
    } else {
      reader.seek(attLen, Reader::SeekCur);      
    }
  }
   
  reader.cursor = start;
  exploreOpcodes(reader, codeLen);

  if (returnType != Type::getVoidTy(*llvmContext)) {
    endNode = PHINode::Create(returnType, "", endBlock);
  }

  reader.cursor = start;
  compileOpcodes(reader, codeLen);
  
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
  
  removeUnusedObjects(objectLocals, intrinsics, TheCompiler->useCooperativeGC());
  removeUnusedObjects(objectStack, intrinsics, TheCompiler->useCooperativeGC());


  delete[] opcodeInfos;
  return endNode;
    
}

llvm::Function* JavaJIT::javaCompile() {
  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "compiling %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());

  DbgSubprogram = TheCompiler->GetDbgSubprogram(compilingMethod);

  Attribut* codeAtt = compilingMethod->lookupAttribut(Attribut::codeAttribut);
  
  if (!codeAtt) {
    fprintf(stderr, "I haven't verified your class file and it's malformed:"
                    " no code attribute found for %s.%s!\n",
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

  opcodeInfos = new Opinfo[codeLen];
  memset(opcodeInfos, 0, codeLen * sizeof(Opinfo));
  for (uint32 i = 0; i < codeLen; ++i) {
    opcodeInfos[i].exceptionBlock = endExceptionBlock;
  }

  Instruction* returnValue = NULL;
  if (returnType == intrinsics->JavaObjectType &&
      TheCompiler->useCooperativeGC()) {
    returnValue = new AllocaInst(intrinsics->JavaObjectType, "",
                                 currentBlock);
    Instruction* cast = 
        new BitCastInst(returnValue, intrinsics->ptrPtrType, "", currentBlock);
    Value* GCArgs[2] = { cast, intrinsics->constantPtrNull };
        
    CallInst::Create(intrinsics->llvm_gc_gcroot, GCArgs, GCArgs + 2, "",
        currentBlock);
  }

  for (int i = 0; i < maxLocals; i++) {
    intLocals.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getInt32Ty(*llvmContext)), intLocals.back(), false, currentBlock);
    doubleLocals.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getDoubleTy(*llvmContext)), doubleLocals.back(), false, currentBlock);
    longLocals.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getInt64Ty(*llvmContext)), longLocals.back(), false, currentBlock);
    floatLocals.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", currentBlock));
    new StoreInst(Constant::getNullValue(Type::getFloatTy(*llvmContext)), floatLocals.back(), false, currentBlock);
    objectLocals.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                          currentBlock));
    // The GCStrategy will already initialize the value.
    if (!TheCompiler->useCooperativeGC())
      new StoreInst(Constant::getNullValue(intrinsics->JavaObjectType), objectLocals.back(), false, currentBlock);
  }
  
  for (int i = 0; i < maxStack; i++) {
    objectStack.push_back(new AllocaInst(intrinsics->JavaObjectType, "",
                                         currentBlock));
    addHighLevelType(objectStack.back(), upcalls->OfObject);
    intStack.push_back(new AllocaInst(Type::getInt32Ty(*llvmContext), "", currentBlock));
    doubleStack.push_back(new AllocaInst(Type::getDoubleTy(*llvmContext), "", currentBlock));
    longStack.push_back(new AllocaInst(Type::getInt64Ty(*llvmContext), "", currentBlock));
    floatStack.push_back(new AllocaInst(Type::getFloatTy(*llvmContext), "", currentBlock));
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
    Instruction* V = new StoreInst(i, objectLocals[0], false, currentBlock);
    addHighLevelType(V, compilingClass);
    ++i;
    ++index;
    ++count;
    thisObject = objectLocals[0];
  }

  for (;count < max; ++i, ++index, ++count, ++type) {
    
    const Typedef* cur = arguments[type];
    const llvm::Type* curType = i->getType();

    if (curType == Type::getInt64Ty(*llvmContext)){
      new StoreInst(i, longLocals[index], false, currentBlock);
      ++index;
    } else if (cur->isUnsigned()) {
      new StoreInst(new ZExtInst(i, Type::getInt32Ty(*llvmContext), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt8Ty(*llvmContext) || curType == Type::getInt16Ty(*llvmContext)) {
      new StoreInst(new SExtInst(i, Type::getInt32Ty(*llvmContext), "", currentBlock),
                    intLocals[index], false, currentBlock);
    } else if (curType == Type::getInt32Ty(*llvmContext)) {
      new StoreInst(i, intLocals[index], false, currentBlock);
    } else if (curType == Type::getDoubleTy(*llvmContext)) {
      new StoreInst(i, doubleLocals[index], false, currentBlock);
      ++index;
    } else if (curType == Type::getFloatTy(*llvmContext)) {
      new StoreInst(i, floatLocals[index], false, currentBlock);
    } else {
      Instruction* V = new StoreInst(i, objectLocals[index], false, currentBlock);
      addHighLevelType(V, cur->findAssocClass(compilingClass->classLoader));
    }
  }

  // Now that arguments have been setup, we can proceed with runtime calls.
#if JNJVM_EXECUTE > 0
    {
    Value* arg = TheCompiler->getMethodInClass(compilingMethod);

    llvm::CallInst::Create(intrinsics->PrintMethodStartFunction, arg, "",
                           currentBlock);
    }
#endif

#if defined(ISOLATE_SHARING)
  ctpCache = i;
  Value* addrCtpCache = new AllocaInst(intrinsics->ConstantPoolType, "",
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
    threadId = getCurrentThread(intrinsics->MutatorThreadType);
     
    IsolateIDPtr = GetElementPtrInst::Create(threadId, intrinsics->constantThree,
                                             "", currentBlock);
    const Type* realType = PointerType::getUnqual(intrinsics->pointerSizeType);
    IsolateIDPtr = new BitCastInst(IsolateIDPtr, realType, "",
                                   currentBlock);
    OldIsolateID = new LoadInst(IsolateIDPtr, "", currentBlock);

    Value* MyID = ConstantInt::get(intrinsics->pointerSizeType,
                                   loader->getIsolate()->IsolateID);
    Cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, OldIsolateID, MyID,
                       "");

    BasicBlock* EndBB = createBasicBlock("After service check");
    BasicBlock* ServiceBB = createBasicBlock("Begin service call");

    BranchInst::Create(EndBB, ServiceBB, Cmp, currentBlock);

    currentBlock = ServiceBB;
  
    new StoreInst(MyID, IsolateIDPtr, currentBlock);
    IsolatePtr = GetElementPtrInst::Create(threadId, intrinsics->constantFour, "",
                                           currentBlock);
     
    OldIsolate = new LoadInst(IsolatePtr, "", currentBlock);
    NewIsolate = intrinsics->getIsolate(loader->getIsolate(), currentBlock);
    new StoreInst(NewIsolate, IsolatePtr, currentBlock);

#if DEBUG
    Value* GEP[2] = { OldIsolate, NewIsolate };
    CallInst::Create(intrinsics->ServiceCallStartFunction, GEP, GEP + 2,
                     "", currentBlock);
#endif
    BranchInst::Create(EndBB, currentBlock);
    currentBlock = EndBB;
  }
#endif

  readExceptionTable(reader, codeLen);
  
  // Lookup line number table attribute.
  uint16 nba = reader.readU2();
  for (uint16 i = 0; i < nba; ++i) {
    const UTF8* attName = compilingClass->ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    if (attName->equals(Attribut::lineNumberTableAttribut)) {
      uint16 lineLength = reader.readU2();
      for (uint16 i = 0; i < lineLength; ++i) {
        uint16 pc = reader.readU2();
        uint16 ln = reader.readU2();
        opcodeInfos[pc].lineNumber = ln;
      }
    } else {
      reader.seek(attLen, Reader::SeekCur);      
    }
  }
  
  reader.cursor = start;
  exploreOpcodes(reader, codeLen);
 
  endBlock = createBasicBlock("end");

  if (returnType != Type::getVoidTy(*llvmContext)) {
    endNode = llvm::PHINode::Create(returnType, "", endBlock);
  }
  
  if (isSynchro(compilingMethod->access)) {
    beginSynchronize();
  }
  
  if (TheCompiler->useCooperativeGC()) {
    Value* threadId = getCurrentThread(intrinsics->MutatorThreadType);
    
    Value* GEP[2] = { intrinsics->constantZero, 
                      intrinsics->OffsetDoYieldInThreadConstant };
    
    Value* YieldPtr = GetElementPtrInst::Create(threadId, GEP, GEP + 2, "",
                                                currentBlock);

    Value* Yield = new LoadInst(YieldPtr, "", currentBlock);

    BasicBlock* continueBlock = createBasicBlock("After safe point");
    BasicBlock* yieldBlock = createBasicBlock("In safe point");
    BranchInst::Create(yieldBlock, continueBlock, Yield, currentBlock);

    currentBlock = yieldBlock;
    CallInst::Create(intrinsics->conditionalSafePoint, "", currentBlock);
    BranchInst::Create(continueBlock, currentBlock);

    currentBlock = continueBlock;
  }
  
  if (TheCompiler->hasExceptionsEnabled()) {
    // Variables have been allocated and the lock has been taken. Do the stack
    // check now: if there is an exception, we will go to the lock release code.
    currentExceptionBlock = opcodeInfos[0].exceptionBlock;
    Value* FrameAddr = CallInst::Create(intrinsics->llvm_frameaddress,
                                       	intrinsics->constantZero, "", currentBlock);
    FrameAddr = new PtrToIntInst(FrameAddr, intrinsics->pointerSizeType, "",
                                 currentBlock);
    Value* stackCheck = 
      BinaryOperator::CreateAnd(FrameAddr, intrinsics->constantStackOverflowMask,
                                "", currentBlock);

    stackCheck = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, stackCheck,
                              intrinsics->constantPtrZero, "");
    BasicBlock* stackOverflow = createBasicBlock("stack overflow");
    BasicBlock* noStackOverflow = createBasicBlock("no stack overflow");
    BranchInst::Create(stackOverflow, noStackOverflow, stackCheck,
                       currentBlock);
    currentBlock = stackOverflow;
    throwException(intrinsics->StackOverflowErrorFunction, 0, 0);
    currentBlock = noStackOverflow;
  }

  reader.cursor = start;
  compileOpcodes(reader, codeLen);
  
  assert(stack.size() == 0 && "Stack not empty after compiling bytecode");
  // Fix a javac(?) bug where a method only throws an exception and does
  // not return.
  pred_iterator PI = pred_begin(endBlock);
  pred_iterator PE = pred_end(endBlock);
  if (PI == PE && returnType != Type::getVoidTy(*llvmContext)) {
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

  if (returnValue != NULL) {
    new StoreInst(endNode, returnValue, currentBlock);
  }
  
  if (isSynchro(compilingMethod->access)) {
    endSynchronize();
  }

#if JNJVM_EXECUTE > 0
    {
    Value* arg = TheCompiler->getMethodInClass(compilingMethod); 
    CallInst::Create(intrinsics->PrintMethodEndFunction, arg, "", currentBlock);
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
    CallInst::Create(intrinsics->ServiceCallStopFunction, GEP, GEP + 2,
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
    if (returnType != Type::getVoidTy(*llvmContext)) {
      if (returnValue != NULL) {
        Value* obj = new LoadInst(
            returnValue, "", TheCompiler->useCooperativeGC(), currentBlock);
        ReturnInst::Create(*llvmContext, obj, currentBlock);
      } else {
        ReturnInst::Create(*llvmContext, endNode, currentBlock);
      }
    } else {
      ReturnInst::Create(*llvmContext, currentBlock);
    }
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
  
  removeUnusedObjects(objectLocals, intrinsics, TheCompiler->useCooperativeGC());
  removeUnusedObjects(objectStack, intrinsics, TheCompiler->useCooperativeGC());
 
  delete[] opcodeInfos;

  PRINT_DEBUG(JNJVM_COMPILE, 1, COLOR_NORMAL, "--> end compiling %s.%s\n",
              UTF8Buffer(compilingClass->name).cString(),
              UTF8Buffer(compilingMethod->name).cString());
  
#ifndef DWARF_EXCEPTIONS
  if (codeLen < 5 && !callsStackWalker && !TheCompiler->isStaticCompiling())
    compilingMethod->canBeInlined = false;
#endif
  
  Attribut* annotationsAtt =
    compilingMethod->lookupAttribut(Attribut::annotationsAttribut);
  
  if (annotationsAtt) {
    Reader reader(annotationsAtt, &(compilingClass->bytes));
    AnnotationReader AR(reader, compilingClass);
    uint16 numAnnotations = reader.readU2();
    for (uint16 i = 0; i < numAnnotations; ++i) {
      AR.readAnnotation();
      const UTF8* name =
        compilingClass->ctpInfo->UTF8At(AR.AnnotationNameIndex);
      if (name->equals(TheCompiler->InlinePragma)) {
        llvmFunction->addFnAttr(Attribute::AlwaysInline);
      } else if (name->equals(TheCompiler->NoInlinePragma)) {
        llvmFunction->addFnAttr(Attribute::NoInline);
      }
    }
  }
 
  if (codeInfo.size()) { 
    compilingMethod->codeInfo = new CodeLineInfo[codeInfo.size()];
    for (uint32 i = 0; i < codeInfo.size(); i++) {
      compilingMethod->codeInfo[i].lineNumber = codeInfo[i].lineNumber;
      compilingMethod->codeInfo[i].ctpIndex = codeInfo[i].ctpIndex;
      compilingMethod->codeInfo[i].bytecodeIndex = codeInfo[i].bytecodeIndex;
      compilingMethod->codeInfo[i].bytecode = codeInfo[i].bytecode;
    }
  } else {
    compilingMethod->codeInfo = NULL;
  }

  return llvmFunction;
}

void JavaJIT::compareFP(Value* val1, Value* val2, const Type* ty, bool l) {
  Value* one = intrinsics->constantOne;
  Value* zero = intrinsics->constantZero;
  Value* minus = intrinsics->constantMinusOne;

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
      push(val, false, upcalls->newString);
    } else {
      JavaString** str = (JavaString**)ctpInfo->ctpRes[index];
      if (str) {
        Value* val = TheCompiler->getStringPtr(str);
        val = new LoadInst(val, "", currentBlock);
        push(val, false, upcalls->newString);
      } else {
        // Lookup the constant pool cache
        const llvm::Type* Ty = PointerType::getUnqual(intrinsics->JavaObjectType);
        Value* val = getConstantPoolAt(index, intrinsics->StringLookupFunction,
                                       Ty, 0, false);
        val = new LoadInst(val, "", currentBlock);
        push(val, false, upcalls->newString);
      }
    }
#endif   
  } else if (type == JavaConstantPool::ConstantLong) {
    push(ConstantInt::get(Type::getInt64Ty(*llvmContext), ctpInfo->LongAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantDouble) {
    push(ConstantFP::get(Type::getDoubleTy(*llvmContext), ctpInfo->DoubleAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantInteger) {
    push(ConstantInt::get(Type::getInt32Ty(*llvmContext), ctpInfo->IntegerAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantFloat) {
    push(ConstantFP::get(Type::getFloatTy(*llvmContext), ctpInfo->FloatAt(index)),
         false);
  } else if (type == JavaConstantPool::ConstantClass) {
    UserCommonClass* cl = 0;
    Value* res = getResolvedCommonClass(index, false, &cl);

    res = CallInst::Create(intrinsics->GetClassDelegateeFunction, res, "",
                           currentBlock);
    push(res, false, upcalls->newClass);
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
    Constant* zero = intrinsics->JavaObjectNullConstant;
    Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, obj, zero, "");

    BasicBlock* exit = createBasicBlock("verifyNullExit");
    BasicBlock* cont = createBasicBlock("verifyNullCont");

    BranchInst::Create(exit, cont, test, currentBlock);
    currentBlock = exit;
    throwException(intrinsics->NullPointerExceptionFunction, 0, 0);
    currentBlock = cont;
  }
 
}

Value* JavaJIT::verifyAndComputePtr(Value* obj, Value* index,
                                    const Type* arrayType, bool verif) {
  JITVerifyNull(obj);
  
  if (index->getType() != Type::getInt32Ty(*llvmContext)) {
    index = new SExtInst(index, Type::getInt32Ty(*llvmContext), "", currentBlock);
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
    throwException(intrinsics->IndexOutOfBoundsExceptionFunction, args, 2);
    currentBlock = ifTrue;
  }
  
  Constant* zero = intrinsics->constantZero;
  Value* val = new BitCastInst(obj, arrayType, "", currentBlock);
  
  Value* indexes[3] = { zero, intrinsics->JavaArrayElementsOffsetConstant, index };
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
  mvm::ThreadAllocator threadAllocator;
  Value** args = (Value**)threadAllocator.Allocate(nb*sizeof(Value*));
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
    if (it->get() == Type::getInt64Ty(*llvmContext) || it->get() == Type::getDoubleTy(*llvmContext)) {
      pop();
    }
    Value* tmp = pop();
    
    const Type* type = it->get();
    if (tmp->getType() != type) { // int8 or int16
      convertValue(tmp, type, currentBlock, false);
    }
    args[i] = tmp;

  }

  for (uint32 i = 0; i < nb; ++i) {
    Args.push_back(args[i]);
  }
}

Value* JavaJIT::getTarget(FunctionType::param_iterator it, uint32 nb) {
#if defined(ISOLATE_SHARING)
  nb += 1;
#endif
#if defined(ISOLATE_SHARING)
  sint32 start = nb - 2;
  it--;
  it--;
#else
  sint32 start = nb - 1;
#endif
  uint32 nbObjects = 0;
  for (sint32 i = start; i >= 0; --i) {
    it--;
    if (it->get() == intrinsics->JavaObjectType) {
      nbObjects++;
    }
  }
  assert(nbObjects > 0 && "No this");
  return objectStack[currentStackIndex - nbObjects];
}

Instruction* JavaJIT::lowerMathOps(const UTF8* name, 
                                   std::vector<Value*>& args) {
  JnjvmBootstrapLoader* loader = compilingClass->classLoader->bootstrapLoader;
  if (name->equals(loader->abs)) {
    const Type* Ty = args[0]->getType();
    if (Ty == Type::getInt32Ty(*llvmContext)) {
      Constant* const_int32_9 = intrinsics->constantZero;
      Constant* const_int32_10 = intrinsics->constantMinusOne;
      BinaryOperator* int32_tmpneg = 
        BinaryOperator::Create(Instruction::Sub, const_int32_9, args[0],
                               "tmpneg", currentBlock);
      ICmpInst* int1_abscond = 
        new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT, args[0], const_int32_10,
                     "abscond");
      return llvm::SelectInst::Create(int1_abscond, args[0], int32_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::getInt64Ty(*llvmContext)) {
      Constant* const_int64_9 = intrinsics->constantLongZero;
      Constant* const_int64_10 = intrinsics->constantLongMinusOne;
      
      BinaryOperator* int64_tmpneg = 
        BinaryOperator::Create(Instruction::Sub, const_int64_9, args[0],
                               "tmpneg", currentBlock);

      ICmpInst* int1_abscond = new ICmpInst(*currentBlock, ICmpInst::ICMP_SGT,
                                            args[0], const_int64_10, "abscond");
      
      return llvm::SelectInst::Create(int1_abscond, args[0], int64_tmpneg,
                                      "abs", currentBlock);
    } else if (Ty == Type::getFloatTy(*llvmContext)) {
      return llvm::CallInst::Create(intrinsics->func_llvm_fabs_f32, args[0],
                                    "tmp1", currentBlock);
    } else if (Ty == Type::getDoubleTy(*llvmContext)) {
      return llvm::CallInst::Create(intrinsics->func_llvm_fabs_f64, args[0],
                                    "tmp1", currentBlock);
    }
  } else if (name->equals(loader->sqrt)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_sqrt_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->sin)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_sin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->cos)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_cos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->tan)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_tan_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->asin)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_asin_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->acos)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_acos_f64, args[0], 
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->atan)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_atan_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->atan2)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_atan2_f64, 
                                  args.begin(), args.end(), "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->exp)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_exp_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_log_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->pow)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_pow_f64, args.begin(),
                                  args.end(), "tmp1", currentBlock);
  } else if (name->equals(loader->ceil)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_ceil_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->floor)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_floor_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->rint)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_rint_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->cbrt)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_cbrt_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->cosh)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_cosh_f64, args[0], "tmp1",
                                  currentBlock);
  } else if (name->equals(loader->expm1)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_expm1_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->hypot)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_hypot_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log10)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_log10_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->log1p)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_log1p_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->sinh)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_sinh_f64, args[0],
                                  "tmp1", currentBlock);
  } else if (name->equals(loader->tanh)) {
    return llvm::CallInst::Create(intrinsics->func_llvm_tanh_f64, args[0],
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

void JavaJIT::invokeSpecial(uint16 index) {
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  JavaMethod* meth = 0;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* cl = 0;

  ctpInfo->nameOfStaticOrSpecialMethod(index, cl, name, signature);
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* virtualType = LSI->getVirtualType();
  llvm::Instruction* val = 0;

#if defined(ISOLATE_SHARING)
  const Type* Ty = intrinsics->ConstantPoolType;
  Constant* Nil = Constant::getNullValue(Ty);
  GlobalVariable* GV = new GlobalVariable(*llvmFunction->getParent(),Ty, false,
                                          GlobalValue::ExternalLinkage, Nil,
                                          "");
  Value* res = new LoadInst(GV, "", false, currentBlock);
  Value* test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, res, Nil, "");
 
  BasicBlock* trueCl = createBasicBlock("UserCtp OK");
  BasicBlock* falseCl = createBasicBlock("UserCtp Not OK");
  PHINode* node = llvm::PHINode::Create(Ty, "", trueCl);
  node->addIncoming(res, currentBlock);
  BranchInst::Create(falseCl, trueCl, test, currentBlock);
  std::vector<Value*> Args;
  Args.push_back(ctpCache);
  Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), index));
  Args.push_back(GV);
  res = CallInst::Create(intrinsics->SpecialCtpLookupFunction, Args.begin(),
                         Args.end(), "", falseCl);
  node->addIncoming(res, falseCl);
  BranchInst::Create(trueCl, falseCl);
  currentBlock = trueCl;
  args.push_back(node);
#endif

  if (!meth) {
    meth = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_VIRTUAL, signature);
  }

  Value* func = 0;
  bool needsInit = false;
  if (TheCompiler->needsCallback(meth, &needsInit)) {
    if (needsInit) {
      // Make sure the class is loaded before materializing the method.
      uint32 clIndex = ctpInfo->getClassIndexFromMethod(index);
      UserCommonClass* cl = 0;
      Value* Cl = getResolvedCommonClass(clIndex, false, &cl);
      if (cl == NULL) {
        CallInst::Create(intrinsics->ForceLoadedCheckFunction, Cl, "",
                         currentBlock);
      }
    }
    func = TheCompiler->addCallback(compilingClass, index, signature, false,
                                    currentBlock);
  } else {
    func = TheCompiler->getMethod(meth);
  }

  std::vector<Value*> args;
  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbArguments + 1);
  JITVerifyNull(args[0]); 
  
  if (meth == compilingClass->classLoader->bootstrapLoader->upcalls->InitObject) {
    return;
  }

  if (meth && canBeInlined(meth)) {
    val = invokeInline(meth, args);
  } else {
    val = invoke(func, args, "", currentBlock);
  }
  
  const llvm::Type* retType = virtualType->getReturnType();
  if (retType != Type::getVoidTy(*llvmContext)) {
    if (retType == intrinsics->JavaObjectType) {
      JnjvmClassLoader* JCL = compilingClass->classLoader;
      push(val, false, signature->getReturnType()->findAssocClass(JCL));
    } else {
      push(val, signature->getReturnType()->isUnsigned());
      if (retType == Type::getDoubleTy(*llvmContext) ||
          retType == Type::getInt64Ty(*llvmContext)) {
        push(intrinsics->constantZero, false);
      }
    }
  }
}

void JavaJIT::invokeStatic(uint16 index) {
  JavaConstantPool* ctpInfo = compilingClass->ctpInfo;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* className = 0;
  ctpInfo->nameOfStaticOrSpecialMethod(index, className, name, signature);
  LLVMSignatureInfo* LSI = TheCompiler->getSignatureInfo(signature);
  const llvm::FunctionType* staticType = LSI->getStaticType();
  ctpInfo->markAsStaticCall(index);
  JnjvmBootstrapLoader* loader = compilingClass->classLoader->bootstrapLoader;
  llvm::Instruction* val = 0;
  
  if (className->equals(loader->stackWalkerName)) {
    callsStackWalker = true;
  }

  JavaMethod* meth = ctpInfo->infoOfStaticOrSpecialMethod(index, ACC_STATIC,
                                                          signature);
    

  uint32 clIndex = ctpInfo->getClassIndexFromMethod(index);
  UserClass* cl = 0;
  Value* Cl = getResolvedClass(clIndex, true, true, &cl);
  if (!meth || (cl && needsInitialisationCheck(cl, compilingClass))) {
    CallInst::Create(intrinsics->ForceInitialisationCheckFunction, Cl, "",
                     currentBlock);
  }
  
  Value* func = 0;
  bool needsInit = false;
  if (TheCompiler->needsCallback(meth, &needsInit)) {
    func = TheCompiler->addCallback(compilingClass, index, signature,
                                    true, currentBlock);
  } else {
    func = TheCompiler->getMethod(meth);
  }

#if defined(ISOLATE_SHARING)
  Value* newCtpCache = getConstantPoolAt(index,
                                         intrinsics->StaticCtpLookupFunction,
                                         intrinsics->ConstantPoolType, 0,
                                         false);
#endif
  std::vector<Value*> args; // size = [signature->nbIn + 2]; 
  FunctionType::param_iterator it  = staticType->param_end();
  makeArgs(it, index, args, signature->nbArguments);
#if defined(ISOLATE_SHARING)
  args.push_back(newCtpCache);
#endif

  if (className->equals(loader->mathName)) {
    val = lowerMathOps(name, args);
  }
    
  if (val == NULL) {
    if (meth != NULL && canBeInlined(meth)) {
      val = invokeInline(meth, args);
    } else {
      val = invoke(func, args, "", currentBlock);
    }
  }

  const llvm::Type* retType = staticType->getReturnType();
  if (retType != Type::getVoidTy(*llvmContext)) {
    if (retType == intrinsics->JavaObjectType) {
      JnjvmClassLoader* JCL = compilingClass->classLoader;
      push(val, false, signature->getReturnType()->findAssocClass(JCL));
    } else {
      push(val, signature->getReturnType()->isUnsigned());
      if (retType == Type::getDoubleTy(*llvmContext) ||
          retType == Type::getInt64Ty(*llvmContext)) {
        push(intrinsics->constantZero, false);
      }
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
  Value* Cl = GetElementPtrInst::Create(CTP, intrinsics->ConstantOne, "",
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
  Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), index));
  if (additionalArg) Args.push_back(additionalArg);

  Value* res = 0;
  if (doThrow) {
    res = invoke(intrinsics->GetConstantPoolAtFunction, Args, "",
                 currentBlock);
  } else {
    res = CallInst::Create(intrinsics->GetConstantPoolAtFunction, Args.begin(),
                           Args.end(), "", currentBlock);
  }
  
  const Type* realType = 
    intrinsics->GetConstantPoolAtFunction->getReturnType();
  if (returnType == Type::getInt32Ty(*llvmContext)) {
    return new PtrToIntInst(res, Type::getInt32Ty(*llvmContext), "", currentBlock);
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
        node->getType() != intrinsics->JavaClassArrayType) {
      node = new LoadInst(node, "", currentBlock);
    }
    if (node->getType() != intrinsics->JavaCommonClassType) {
      node = new BitCastInst(node, intrinsics->JavaCommonClassType, "",
                             currentBlock);
    }
  } else {
    node = getConstantPoolAt(index, intrinsics->ClassLookupFunction,
                             intrinsics->JavaCommonClassType, 0, doThrow);
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
    node = getConstantPoolAt(index, intrinsics->ClassLookupFunction,
                             intrinsics->JavaClassType, 0, doThrow);
  }
 

  if (clinit && needsInit) {
    if (node->getType() != intrinsics->JavaClassType) {
      node = new BitCastInst(node, intrinsics->JavaClassType, "", currentBlock);
    }
    return invoke(intrinsics->InitialisationCheckFunction, node, "",
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
      Cl = invoke(intrinsics->ForceInitialisationCheckFunction, Cl, "",
                  currentBlock);
    }

  } else {
    VT = CallInst::Create(intrinsics->GetVTFromClassFunction, Cl, "",
                          currentBlock);
    Size = CallInst::Create(intrinsics->GetObjectSizeFromClassFunction, Cl,
                            "", currentBlock);
  }
 
  VT = new BitCastInst(VT, intrinsics->ptrType, "", currentBlock);
  Instruction* val = invoke(cl ? intrinsics->AllocateFunction :
                           intrinsics->AllocateUnresolvedFunction,
                           Size, VT, "", currentBlock);

  if (cl && cl->virtualVT->destructor) {
    CallInst::Create(intrinsics->AddFinalizationCandidate, val, "", currentBlock);
  }


  addHighLevelType(val, cl ? cl : upcalls->OfObject);
  val = new BitCastInst(val, intrinsics->JavaObjectType, "", currentBlock);
  push(val, false, cl ? cl : upcalls->OfObject);
}

Value* JavaJIT::ldResolved(uint16 index, bool stat, Value* object, 
                           const Type* fieldTypePtr) {
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
        Cl = invoke(intrinsics->InitialisationCheckFunction, Cl, "",
                    currentBlock);
      }
#if !defined(ISOLATE) && !defined(ISOLATE_SHARING)
      if (needsCheck) {
        CallInst::Create(intrinsics->ForceInitialisationCheckFunction, Cl, "",
                         currentBlock);
      }

      object = TheCompiler->getStaticInstance(field->classDef);
#else
      object = CallInst::Create(intrinsics->GetStaticInstanceFunction, Cl, "",
                                currentBlock); 
#endif
    } else {
      object = new LoadInst(
          object, "", TheCompiler->useCooperativeGC(), currentBlock);
      JITVerifyNull(object);
      type = LCI->getVirtualType();
    }
    
    Value* objectConvert = new BitCastInst(object, type, "", currentBlock);

    Value* args[2] = { intrinsics->constantZero, LFI->getOffset() };
    Value* ptr = llvm::GetElementPtrInst::Create(objectConvert,
                                                 args, args + 2, "",
                                                 currentBlock);
    return ptr;
  }

  const Type* Pty = intrinsics->arrayPtrType;
  Constant* zero = intrinsics->constantZero;
    
  Function* func = stat ? intrinsics->StaticFieldLookupFunction :
                          intrinsics->VirtualFieldLookupFunction;
    
  const Type* returnType = 0;
  if (stat) {
    returnType = intrinsics->ptrType;
  } else {
    returnType = Type::getInt32Ty(*llvmContext);
  }

  Value* ptr = getConstantPoolAt(index, func, returnType, 0, true);
  if (!stat) {
    object = new LoadInst(
        object, "", TheCompiler->useCooperativeGC(), currentBlock);
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
    if (t1->isIntegerTy() && t2->isIntegerTy()) {
      if (t2->getPrimitiveSizeInBits() < t1->getPrimitiveSizeInBits()) {
        if (usign) {
          val = new ZExtInst(val, t1, "", currentBlock);
        } else {
          val = new SExtInst(val, t1, "", currentBlock);
        }
      } else {
        val = new TruncInst(val, t1, "", currentBlock);
      }    
    } else if (t1->isFloatTy() && t2->isFloatTy()) {
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
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
   
  Value* ptr = ldResolved(index, true, NULL, LAI.llvmTypePtr);

  Value* val = pop(); 
  if (type == Type::getInt64Ty(*llvmContext) ||
      type == Type::getDoubleTy(*llvmContext)) {
    val = pop();
  }

  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, false);
  }

  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getStaticField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  
  Value* ptr = ldResolved(index, true, NULL, LAI.llvmTypePtr);
  
  bool final = false;
#if !defined(ISOLATE) && !defined(ISOLATE_SHARING)
  JnjvmBootstrapLoader* JBL = compilingClass->classLoader->bootstrapLoader;
  if (!compilingMethod->name->equals(JBL->clinitName)) {
    JavaField* field = compilingClass->ctpInfo->lookupField(index, true);
    if (field && field->classDef->isReady()) final = isFinal(field->access);
    if (final) {
      if (sign->isPrimitive()) {
        const PrimitiveTypedef* prim = (PrimitiveTypedef*)sign;
        if (prim->isInt()) {
          sint32 val = field->getStaticInt32Field();
          push(ConstantInt::get(Type::getInt32Ty(*llvmContext), val), false);
        } else if (prim->isByte()) {
          sint8 val = (sint8)field->getStaticInt8Field();
          push(ConstantInt::get(Type::getInt8Ty(*llvmContext), val), false);
        } else if (prim->isBool()) {
          uint8 val = (uint8)field->getStaticInt8Field();
          push(ConstantInt::get(Type::getInt8Ty(*llvmContext), val), true);
        } else if (prim->isShort()) {
          sint16 val = (sint16)field->getStaticInt16Field();
          push(ConstantInt::get(Type::getInt16Ty(*llvmContext), val), false);
        } else if (prim->isChar()) {
          uint16 val = (uint16)field->getStaticInt16Field();
          push(ConstantInt::get(Type::getInt16Ty(*llvmContext), val), true);
        } else if (prim->isLong()) {
          sint64 val = (sint64)field->getStaticLongField();
          push(ConstantInt::get(Type::getInt64Ty(*llvmContext), val), false);
        } else if (prim->isFloat()) {
          float val = (float)field->getStaticFloatField();
          push(ConstantFP::get(Type::getFloatTy(*llvmContext), val), false);
        } else if (prim->isDouble()) {
          double val = (double)field->getStaticDoubleField();
          push(ConstantFP::get(Type::getDoubleTy(*llvmContext), val), false);
        } else {
          abort();
        }
      } else {
        if (TheCompiler->isStaticCompiling()) {
          JavaObject* val = field->getStaticObjectField();
          JnjvmClassLoader* JCL = field->classDef->classLoader;
          Value* V = TheCompiler->getFinalObject(val, sign->assocClass(JCL));
          CommonClass* cl = mvm::Collector::begOf(val) ?
              JavaObject::getClass(val) : NULL;
          push(V, false, cl);
        } else {
          // Do not call getFinalObject, as the object may move in-between two
          // loads of this static.
          Value* V = new LoadInst(ptr, "", currentBlock);
          JnjvmClassLoader* JCL = compilingClass->classLoader;
          push(V, false, sign->findAssocClass(JCL));
        } 
      }
    }
  }
#endif

  if (!final) {
    JnjvmClassLoader* JCL = compilingClass->classLoader;
    CommonClass* cl = sign->findAssocClass(JCL);
    push(new LoadInst(ptr, "", currentBlock), sign->isUnsigned(), cl);
  }
  if (type == Type::getInt64Ty(*llvmContext) ||
      type == Type::getDoubleTy(*llvmContext)) {
    push(intrinsics->constantZero, false);
  }
}

void JavaJIT::setVirtualField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  int stackIndex = currentStackIndex - 2;
  if (type == Type::getInt64Ty(*llvmContext) ||
      type == Type::getDoubleTy(*llvmContext)) {
    stackIndex--;
  }
  Value* object = objectStack[stackIndex];
  Value* ptr = ldResolved(index, false, object, LAI.llvmTypePtr);

  Value* val = pop();
  if (type == Type::getInt64Ty(*llvmContext) ||
      type == Type::getDoubleTy(*llvmContext)) {
    val = pop();
  }
  pop(); // Pop the object
  
  if (type != val->getType()) { // int1, int8, int16
    convertValue(val, type, currentBlock, false);
  }

  new StoreInst(val, ptr, false, currentBlock);
}

void JavaJIT::getVirtualField(uint16 index) {
  Typedef* sign = compilingClass->ctpInfo->infoOfField(index);
  JnjvmClassLoader* JCL = compilingClass->classLoader;
  CommonClass* cl = sign->findAssocClass(JCL);
  
  LLVMAssessorInfo& LAI = TheCompiler->getTypedefInfo(sign);
  const Type* type = LAI.llvmType;
  Value* obj = objectStack[currentStackIndex - 1];
  pop(); // Pop the object
  
  Value* ptr = ldResolved(index, false, obj, LAI.llvmTypePtr);
  
  JnjvmBootstrapLoader* JBL = compilingClass->classLoader->bootstrapLoader;
  bool final = false;
  
  // In init methods, the fields have not been set yet.
  if (!compilingMethod->name->equals(JBL->initName)) {
    JavaField* field = compilingClass->ctpInfo->lookupField(index, false);
    if (field) {
      final = isFinal(field->access) && sign->isPrimitive();
    }
    if (final) {
      Function* F = 0;
      assert(sign->isPrimitive());
      const PrimitiveTypedef* prim = (PrimitiveTypedef*)sign;
      if (prim->isInt()) {
        F = intrinsics->GetFinalInt32FieldFunction;
      } else if (prim->isByte()) {
        F = intrinsics->GetFinalInt8FieldFunction;
      } else if (prim->isBool()) {
        F = intrinsics->GetFinalInt8FieldFunction;
      } else if (prim->isShort()) {
        F = intrinsics->GetFinalInt16FieldFunction;
      } else if (prim->isChar()) {
        F = intrinsics->GetFinalInt16FieldFunction;
      } else if (prim->isLong()) {
        F = intrinsics->GetFinalLongFieldFunction;
      } else if (prim->isFloat()) {
        F = intrinsics->GetFinalFloatFieldFunction;
      } else if (prim->isDouble()) {
        F = intrinsics->GetFinalDoubleFieldFunction;
      } else {
        abort();
      }
      push(CallInst::Create(F, ptr, "", currentBlock), sign->isUnsigned(), cl);
    }
  }
 
  if (!final) push(new LoadInst(ptr, "", currentBlock), sign->isUnsigned(), cl);
  if (type == Type::getInt64Ty(*llvmContext) ||
      type == Type::getDoubleTy(*llvmContext)) {
    push(intrinsics->constantZero, false);
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
 
  const llvm::Type* retType = virtualType->getReturnType();
  BasicBlock* endBlock = createBasicBlock("end interface invoke");
  PHINode * node = 0;
  if (retType != Type::getVoidTy(*llvmContext)) {
    node = PHINode::Create(retType, "", endBlock);
  }
  
 
  CommonClass* cl = 0;
  JavaMethod* meth = 0;
  ctpInfo->infoOfMethod(index, ACC_VIRTUAL, cl, meth);
  Value* Meth = 0;

  if (meth) {
    Meth = TheCompiler->getMethodInClass(meth);
  } else {
    Meth = getConstantPoolAt(index, intrinsics->InterfaceLookupFunction,
                             intrinsics->JavaMethodType, 0, true);
  }

  // Compute the arguments after calling getConstantPoolAt because the
  // arguments will be loaded and the runtime may trigger GC.
  std::vector<Value*> args; // size = [signature->nbIn + 3];

  FunctionType::param_iterator it  = virtualType->param_end();
  makeArgs(it, index, args, signature->nbArguments + 1);
  JITVerifyNull(args[0]);

  BasicBlock* label_bb = createBasicBlock("bb");
  BasicBlock* label_bb4 = createBasicBlock("bb4");
  BasicBlock* label_bb6 = createBasicBlock("bb6");
  BasicBlock* label_bb7 = createBasicBlock("bb7");
    
  // Block entry (label_entry)
  Value* VT = CallInst::Create(intrinsics->GetVTFunction, args[0], "",
                               currentBlock);
  Value* IMT = CallInst::Create(intrinsics->GetIMTFunction, VT, "",
                                currentBlock);

  uint32_t tableIndex = InterfaceMethodTable::getIndex(name, signature->keyName);
  Constant* Index = ConstantInt::get(Type::getInt32Ty(*llvmContext),
                                     tableIndex);

  Value* indices[2] = { intrinsics->constantZero, Index };
  Instruction* ptr_18 = GetElementPtrInst::Create(IMT, indices, indices + 2, "",
                                                  currentBlock);
  Instruction* int32_19 = new LoadInst(ptr_18, "", false, currentBlock);
  int32_19 = new PtrToIntInst(int32_19, intrinsics->pointerSizeType, "",
                              currentBlock);
  Value* one = ConstantInt::get(intrinsics->pointerSizeType, 1);
  Value* zero = ConstantInt::get(intrinsics->pointerSizeType, 0);
  BinaryOperator* int32_20 = BinaryOperator::Create(Instruction::And, int32_19,
                                                    one, "", currentBlock);
  ICmpInst* int1_toBool = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ,
                                       int32_20, zero, "toBool");
  BranchInst::Create(label_bb, label_bb4, int1_toBool, currentBlock);
    
  // Block bb (label_bb)
  currentBlock = label_bb;
  CastInst* ptr_22 = new IntToPtrInst(int32_19, virtualPtrType, "", currentBlock);
  Value* ret = invoke(ptr_22, args, "", currentBlock);
  if (node) node->addIncoming(ret, currentBlock);
  BranchInst::Create(endBlock, currentBlock);
    
  // Block bb4 (label_bb4)
  currentBlock = label_bb4;
  Constant* MinusTwo = ConstantInt::get(intrinsics->pointerSizeType, -2);
  BinaryOperator* int32_25 = BinaryOperator::Create(Instruction::And, int32_19,
                                                    MinusTwo, "", currentBlock);
  const PointerType* Ty = PointerType::getUnqual(intrinsics->JavaMethodType);
  CastInst* ptr_26 = new IntToPtrInst(int32_25, Ty, "", currentBlock);
  LoadInst* int32_27 = new LoadInst(ptr_26, "", false, currentBlock);
  ICmpInst* int1_28 = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, int32_27,
                                   Meth, "");
  BranchInst::Create(label_bb6, label_bb7, int1_28, currentBlock);
    
  // Block bb6 (label_bb6)
  currentBlock = label_bb6;
  PHINode* ptr_table_0_lcssa = PHINode::Create(Ty, "table.0.lcssa",
                                               currentBlock);
  ptr_table_0_lcssa->reserveOperandSpace(2);
  ptr_table_0_lcssa->addIncoming(ptr_26, label_bb4);
   
  GetElementPtrInst* ptr_31 = GetElementPtrInst::Create(ptr_table_0_lcssa,
                                                        intrinsics->constantOne, "",
                                                        currentBlock);

  LoadInst* int32_32 = new LoadInst(ptr_31, "", false, currentBlock);
  CastInst* ptr_33 = new BitCastInst(int32_32, virtualPtrType, "",
                                     currentBlock);
  ret = invoke(ptr_33, args, "", currentBlock);
  if (node) node->addIncoming(ret, currentBlock);
  BranchInst::Create(endBlock, currentBlock);
    
  // Block bb7 (label_bb7)
  currentBlock = label_bb7;
  PHINode* int32_indvar = PHINode::Create(Type::getInt32Ty(*llvmContext),
                                          "indvar", currentBlock);
  int32_indvar->reserveOperandSpace(2);
  int32_indvar->addIncoming(intrinsics->constantZero, label_bb4);
    
  BinaryOperator* int32_table_010_rec =
    BinaryOperator::Create(Instruction::Shl, int32_indvar, intrinsics->constantOne,
                           "table.010.rec", currentBlock);

  BinaryOperator* int32__rec =
    BinaryOperator::Create(Instruction::Add, int32_table_010_rec,
                           intrinsics->constantTwo, ".rec", currentBlock);
  GetElementPtrInst* ptr_37 = GetElementPtrInst::Create(ptr_26, int32__rec, "",
                                                        currentBlock);
  LoadInst* int32_38 = new LoadInst(ptr_37, "", false, currentBlock);
  ICmpInst* int1_39 = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, int32_38,
                                   Meth, "");
  BinaryOperator* int32_indvar_next =
    BinaryOperator::Create(Instruction::Add, int32_indvar, intrinsics->constantOne,
                           "indvar.next", currentBlock);
  BranchInst::Create(label_bb6, label_bb7, int1_39, currentBlock);
  
  int32_indvar->addIncoming(int32_indvar_next, currentBlock);
  ptr_table_0_lcssa->addIncoming(ptr_37, currentBlock);
      
  currentBlock = endBlock;
  if (node) {
    if (node->getType() == intrinsics->JavaObjectType) {
      JnjvmClassLoader* JCL = compilingClass->classLoader;
      push(node, false, signature->getReturnType()->findAssocClass(JCL));
    } else {
      push(node, signature->getReturnType()->isUnsigned());
      if (retType == Type::getDoubleTy(*llvmContext) ||
          retType == Type::getInt64Ty(*llvmContext)) {
        push(intrinsics->constantZero, false);
      }
    }
  }
}

void JavaJIT::lowerArraycopy(std::vector<Value*>& args) {
  Function* meth = TheCompiler->getMethod(upcalls->VMSystemArraycopy);

  Value* ptr_src = args[0];
  Value* int32_start = args[1];
  Value* ptr_dst = args[2];
  Value* int32_start2 = args[3];
  Value* int32_length = args[4];

  JITVerifyNull(ptr_src);
  JITVerifyNull(ptr_dst);

  BasicBlock* label_entry = currentBlock;
  BasicBlock* label_bb = createBasicBlock("bb");
  BasicBlock* label_bb2 = createBasicBlock("bb2");
  BasicBlock* label_bb4 = createBasicBlock("bb4");
  BasicBlock* label_bb5 = createBasicBlock("bb5");
  BasicBlock* label_bb12_preheader = createBasicBlock("bb12.preheader");
  BasicBlock* label_bb7 = createBasicBlock("bb7");
  BasicBlock* label_bb11 = createBasicBlock("bb11");
  BasicBlock* label_memmove = createBasicBlock("memmove");
  BasicBlock* label_backward = createBasicBlock("backward");
  BasicBlock* label_return = createBasicBlock("return");
  
  BasicBlock* log_label_entry = createBasicBlock("log_entry");
  BasicBlock* log_label_bb = createBasicBlock("log_bb");
    
  // Block entry (label_entry)
  CallInst* ptr_16 = CallInst::Create(intrinsics->GetVTFunction, ptr_src, "",
                                      label_entry);
  CallInst* ptr_17 = CallInst::Create(intrinsics->GetVTFunction, ptr_dst, "",
                                      label_entry);
  
  ICmpInst* int1_18 = new ICmpInst(*label_entry, ICmpInst::ICMP_EQ, ptr_16,
                                   ptr_17, "");
  BranchInst::Create(label_bb, label_bb2, int1_18, label_entry);
    
  // Block bb (label_bb)
  currentBlock = label_bb;
  CallInst* ptr_20 = CallInst::Create(intrinsics->GetClassFunction, ptr_src, "",
                                      label_bb);
  std::vector<Value*> ptr_21_indices;
  ptr_21_indices.push_back(intrinsics->constantZero);
  ptr_21_indices.push_back(intrinsics->OffsetAccessInCommonClassConstant);
  Instruction* ptr_21 =
    GetElementPtrInst::Create(ptr_20, ptr_21_indices.begin(),
                              ptr_21_indices.end(), "", label_bb);
  LoadInst* int32_22 = new LoadInst(ptr_21, "", false, label_bb);
  Value* cmp = BinaryOperator::CreateAnd(int32_22, intrinsics->IsArrayConstant, "",
                                         label_bb);
  Value* zero = ConstantInt::get(Type::getInt16Ty(*llvmContext), 0);
  ICmpInst* int1_23 = new ICmpInst(*label_bb, ICmpInst::ICMP_NE, cmp, zero, "");
  BranchInst::Create(label_bb4, label_bb2, int1_23, label_bb);
   

  // Block bb2 (label_bb2)
  currentBlock = label_bb2;
  invoke(meth, args, "", label_bb2);
  BranchInst::Create(label_return, currentBlock);
    
    
  // Block bb4 (label_bb4)
  currentBlock = label_bb4;
  BinaryOperator* int32_27 = BinaryOperator::Create(Instruction::Add,
                                                    int32_length, int32_start,
                                                    "", label_bb4);
  Value* int32_28 = arraySize(ptr_src);
    
  ICmpInst* int1_29 = new ICmpInst(*label_bb4, ICmpInst::ICMP_ULE, int32_27,
                                   int32_28, "");
  BranchInst::Create(label_bb5, label_bb7, int1_29, label_bb4);
    
  // Block bb5 (label_bb5)
  currentBlock = label_bb5;
  BinaryOperator* int32_31 = BinaryOperator::Create(Instruction::Add,
                                                    int32_length, int32_start2,
                                                    "", label_bb5);
  Value* int32_32 = arraySize(ptr_dst);
    
  ICmpInst* int1_33 = new ICmpInst(*label_bb5, ICmpInst::ICMP_ULE, int32_31,
                                   int32_32, "");
  BranchInst::Create(label_bb12_preheader, label_bb7, int1_33, label_bb5);
    
  // Block bb12.preheader (label_bb12_preheader)
  currentBlock = label_bb12_preheader;
  ICmpInst* int1_35 = new ICmpInst(*label_bb12_preheader, ICmpInst::ICMP_UGT,
                                   int32_length, intrinsics->constantZero, "");
  BranchInst::Create(log_label_entry, label_return, int1_35, label_bb12_preheader);
    
  // Block bb7 (label_bb7)
  currentBlock = label_bb7;
  Value* VTArgs[1] = { Constant::getNullValue(intrinsics->VTType) };
  throwException(intrinsics->ArrayStoreExceptionFunction, VTArgs, 1);
   

  
  // Block entry (label_entry)
  currentBlock = log_label_entry;
  Value* ptr_10_indices[2] = { intrinsics->constantZero,
                               intrinsics->OffsetBaseClassInArrayClassConstant };
  Instruction* temp = new BitCastInst(ptr_20, intrinsics->JavaClassArrayType, "",
                                      log_label_entry);
  Instruction* ptr_10 = GetElementPtrInst::Create(temp, ptr_10_indices,
                                                  ptr_10_indices + 2, "",
                                                  log_label_entry);

  LoadInst* ptr_11 = new LoadInst(ptr_10, "", false, log_label_entry);
    
  Value* ptr_12_indices[2] = { intrinsics->constantZero,
                               intrinsics->OffsetAccessInCommonClassConstant };
  Instruction* ptr_12 = GetElementPtrInst::Create(ptr_11, ptr_12_indices,
                                                  ptr_12_indices + 2, "",
                                                  log_label_entry);
  LoadInst* int16_13 = new LoadInst(ptr_12, "", false, log_label_entry);

  BinaryOperator* int32_15 = BinaryOperator::Create(Instruction::And, int16_13,
                                                    intrinsics->IsPrimitiveConstant,
                                                    "", log_label_entry);
  ICmpInst* int1_16 = new ICmpInst(*log_label_entry, ICmpInst::ICMP_EQ,
                                   int32_15, zero, "");
  BranchInst::Create(label_bb2, log_label_bb, int1_16, log_label_entry);
    
  // Block bb (log_label_bb)
  currentBlock = log_label_bb;
  Value* ptr_11_indices[2] = { intrinsics->constantZero,
                               intrinsics->OffsetLogSizeInPrimitiveClassConstant };
  temp = new BitCastInst(ptr_11, intrinsics->JavaClassPrimitiveType, "",
                         log_label_bb);
  GetElementPtrInst* ptr_18 = GetElementPtrInst::Create(temp, ptr_11_indices,
                                                        ptr_11_indices + 2, "",
                                                        log_label_bb);
  LoadInst* int32_20 = new LoadInst(ptr_18, "", false, log_label_bb);
   
  int32_start = BinaryOperator::CreateShl(int32_start, int32_20, "",
                                          log_label_bb);
  int32_start2 = BinaryOperator::CreateShl(int32_start2, int32_20, "",
                                           log_label_bb);
  int32_length = BinaryOperator::CreateShl(int32_length, int32_20, "",
                                           log_label_bb);

  ptr_src = new BitCastInst(ptr_src, intrinsics->JavaArrayUInt8Type, "",
                            log_label_bb);
  
  ptr_dst = new BitCastInst(ptr_dst, intrinsics->JavaArrayUInt8Type, "",
                            log_label_bb);
  
  Value* indexes[3] = { intrinsics->constantZero,
                        intrinsics->JavaArrayElementsOffsetConstant,
                        int32_start };
  Instruction* ptr_42 = GetElementPtrInst::Create(ptr_src, indexes, indexes + 3,
                                                  "", log_label_bb);
  
  indexes[2] = int32_start2;
  Instruction* ptr_44 = GetElementPtrInst::Create(ptr_dst, indexes, indexes + 3,
                                                  "", log_label_bb);
 
  BranchInst::Create(label_memmove, log_label_bb);

  // Block memmove
  currentBlock = label_memmove;
  Value* src_int = new PtrToIntInst(ptr_42, intrinsics->pointerSizeType, "",
                                    currentBlock);
  
  Value* dst_int = new PtrToIntInst(ptr_44, intrinsics->pointerSizeType, "",
                                    currentBlock);

  cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_ULT, dst_int, src_int, "");

  Value* increment = SelectInst::Create(cmp, intrinsics->constantOne,
                                        intrinsics->constantMinusOne, "",
                                        currentBlock);
  BranchInst::Create(label_bb11, label_backward, cmp, currentBlock);

  PHINode* phi_dst_ptr = PHINode::Create(ptr_44->getType(), "", label_bb11);
  PHINode* phi_src_ptr = PHINode::Create(ptr_44->getType(), "", label_bb11);
  phi_dst_ptr->addIncoming(ptr_44, currentBlock);
  phi_src_ptr->addIncoming(ptr_42, currentBlock);
 
  // Block backward
  currentBlock = label_backward;

  ptr_42 = GetElementPtrInst::Create(ptr_42, int32_length, "",
                                     currentBlock);
  
  ptr_44 = GetElementPtrInst::Create(ptr_44, int32_length, "",
                                     currentBlock);
  
  ptr_42 = GetElementPtrInst::Create(ptr_42, intrinsics->constantMinusOne, "",
                                     currentBlock);
  
  ptr_44 = GetElementPtrInst::Create(ptr_44, intrinsics->constantMinusOne, "",
                                     currentBlock);
  
  phi_dst_ptr->addIncoming(ptr_44, currentBlock);
  phi_src_ptr->addIncoming(ptr_42, currentBlock);

  BranchInst::Create(label_bb11, currentBlock);
  
  // Block bb11 (label_bb11)
  currentBlock = label_bb11;
  Argument* fwdref_39 = new Argument(Type::getInt32Ty(*llvmContext));
  PHINode* int32_i_016 = PHINode::Create(Type::getInt32Ty(*llvmContext),
                                         "i.016", label_bb11);
  int32_i_016->reserveOperandSpace(2);
  int32_i_016->addIncoming(fwdref_39, label_bb11);
  int32_i_016->addIncoming(intrinsics->constantZero, log_label_bb);
   
  LoadInst* ptr_43 = new LoadInst(phi_src_ptr, "", false, label_bb11);
  new StoreInst(ptr_43, phi_dst_ptr, false, label_bb11);


  ptr_42 = GetElementPtrInst::Create(phi_src_ptr, increment, "",
                                     label_bb11);
  
  ptr_44 = GetElementPtrInst::Create(phi_dst_ptr, increment, "",
                                     label_bb11);
  phi_dst_ptr->addIncoming(ptr_44, label_bb11);
  phi_src_ptr->addIncoming(ptr_42, label_bb11);

  BinaryOperator* int32_indvar_next =
    BinaryOperator::Create(Instruction::Add, int32_i_016, intrinsics->constantOne,
                           "indvar.next", label_bb11);
  ICmpInst* int1_exitcond = new ICmpInst(*label_bb11, ICmpInst::ICMP_EQ,
                                         int32_indvar_next, int32_length,
                                         "exitcond");
  BranchInst::Create(label_return, label_bb11, int1_exitcond, label_bb11);

  // Resolve Forward References
  fwdref_39->replaceAllUsesWith(int32_indvar_next); delete fwdref_39;

  currentBlock = label_return;
}

DebugLoc JavaJIT::CreateLocation() {
  LineInfo LI = { currentLineNumber, currentCtpIndex, currentBytecodeIndex,
                  currentBytecode };
  codeInfo.push_back(LI);
  DebugLoc DL = DebugLoc::get(callNumber++, 0, DbgSubprogram);
  return DL;
}

#ifdef DWARF_EXCEPTIONS
#include "ExceptionsDwarf.inc"
#else
#include "ExceptionsCheck.inc"
#endif
