//===------ JavaCache.cpp - Inline cache for virtual calls -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <vector>

#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "types.h"

using namespace jnjvm;
using namespace llvm;

void Enveloppe::destroyer(size_t sz) {
  delete cacheLock;
}

void CacheNode::print(mvm::PrintBuffer* buf) const {
  buf->write("CacheNode<");
  if (lastCible) {
    lastCible->print(buf);
    buf->write(" -- ");
    ((mvm::Object*)((void**)methPtr - 1))->print(buf);
  }
  buf->write(" in ");
  enveloppe->print(buf);
  buf->write(">");
}

void Enveloppe::print(mvm::PrintBuffer* buf) const {
  buf->write("Enveloppe<>");
}

void CacheNode::initialise() {
  this->lastCible = 0;
  this->methPtr = 0;
  this->next = 0;
}

Enveloppe* Enveloppe::allocate(JavaCtpInfo* ctp, uint32 index) {
  Enveloppe* enveloppe = vm_new(ctp->classDef->isolate, Enveloppe)();
  enveloppe->firstCache = vm_new(ctp->classDef->isolate, CacheNode)();
  enveloppe->firstCache->initialise();
  enveloppe->firstCache->enveloppe = enveloppe;
  enveloppe->cacheLock = mvm::Lock::allocNormal();
  enveloppe->ctpInfo = ctp;
  enveloppe->index = index;
  return enveloppe;
}

void JavaJIT::invokeInterfaceOrVirtual(uint16 index) {
  
  // Do the usual
  JavaCtpInfo* ctpInfo = compilingClass->ctpInfo;
  Signdef* signature = ctpInfo->infoOfInterfaceOrVirtualMethod(index);
  
  std::vector<Value*> args; // size = [signature->nbIn + 3];

  FunctionType::param_iterator it  = signature->virtualType->param_end();
  makeArgs(it, index, args, signature->nbIn + 1);
  
  const llvm::Type* retType = signature->virtualType->getReturnType();
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
  
  mvm::jit::protectConstants();//->lock();
  Value* llvmEnv = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (enveloppe)),
                              Enveloppe::llvmType);
  mvm::jit::unprotectConstants();//->unlock();
  

  JITVerifyNull(args[0]);

  std::vector<Value*> args1;
  args1.push_back(zero);
  args1.push_back(one);
  Value* cachePtr = GetElementPtrInst::Create(llvmEnv, args1.begin(), args1.end(),
                                          "", currentBlock);
  Value* cache = new LoadInst(cachePtr, "", currentBlock);

  std::vector<Value*> args2;
  args2.push_back(zero);
  args2.push_back(JavaObject::classOffset());
  Value* classPtr = GetElementPtrInst::Create(args[0], args2.begin(),
                                          args2.end(), "",
                                          currentBlock);

  Value* cl = new LoadInst(classPtr, "", currentBlock);
  std::vector<Value*> args3;
  args3.push_back(zero);
  args3.push_back(two);
  Value* lastCiblePtr = GetElementPtrInst::Create(cache, args3.begin(), args3.end(),
                                              "", currentBlock);
  Value* lastCible = new LoadInst(lastCiblePtr, "", currentBlock);

  Value* cmp = new ICmpInst(ICmpInst::ICMP_EQ, cl, lastCible, "", currentBlock);
  
  BasicBlock* ifTrue = createBasicBlock("cache ok");
  BasicBlock* ifFalse = createBasicBlock("cache not ok");
  BranchInst::Create(ifTrue, ifFalse, cmp, currentBlock);
  
  currentBlock = ifFalse;
  Value* _meth = invoke(virtualLookupLLVM, cache, args[0], "", ifFalse);
  Value* meth = new BitCastInst(_meth, signature->virtualTypePtr, "", 
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
  meth = new BitCastInst(_meth, signature->virtualTypePtr, "", currentBlock);
  
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
