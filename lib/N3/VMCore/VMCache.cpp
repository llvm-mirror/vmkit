//===------- VMCache.cpp - Inline cache for virtual calls -----------------===//
//
//                              N3
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

#include "Assembly.h"
#include "CLIJit.h"
#include "N3.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMCache.h"
#include "VMClass.h"
#include "VMThread.h"

#include "types.h"

using namespace n3;
using namespace llvm;

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

CacheNode* CacheNode::allocate() {
  CacheNode* cache = gc_new(CacheNode)();
  cache->lastCible = 0;
  cache->methPtr = 0;
  cache->next = 0;
  return cache;
}

Enveloppe* Enveloppe::allocate(VMMethod* meth) {
  Enveloppe* enveloppe = gc_new(Enveloppe)();
  enveloppe->firstCache = CacheNode::allocate();
  enveloppe->firstCache->enveloppe = enveloppe;
  enveloppe->cacheLock = mvm::Lock::allocNormal();
  enveloppe->originalMethod = meth;
  return enveloppe;
}

void CLIJit::invokeInterfaceOrVirtual(uint32 value) {
  
  VMMethod* origMeth = compilingClass->assembly->getMethodFromToken(value);
  const llvm::FunctionType* funcType = origMeth->getSignature();
  
  std::vector<Value*> args;
  makeArgs(funcType, args, origMeth->structReturn);

  BasicBlock* callBlock = createBasicBlock("call virtual invoke");
  PHINode* node = PHINode::Create(CacheNode::llvmType, "", callBlock);
  
  Value* argObj = args[0];
  if (argObj->getType() != VMObject::llvmType) {
    argObj = new BitCastInst(argObj, VMObject::llvmType, "", currentBlock);
  }
  JITVerifyNull(argObj);

  // ok now the cache
  Enveloppe* enveloppe = Enveloppe::allocate(origMeth);
  compilingMethod->caches.push_back(enveloppe);
  
  Value* zero = mvm::jit::constantZero;
  Value* one = mvm::jit::constantOne;
  Value* two = mvm::jit::constantTwo;
  Value* five = mvm::jit::constantFive;
  
  mvm::jit::protectConstants();
  Value* llvmEnv = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (enveloppe)),
                  Enveloppe::llvmType);
  mvm::jit::unprotectConstants();
  
  std::vector<Value*> args1;
  args1.push_back(zero);
  args1.push_back(one);
  Value* cachePtr = GetElementPtrInst::Create(llvmEnv, args1.begin(), args1.end(),
                                          "", currentBlock);
  Value* cache = new LoadInst(cachePtr, "", currentBlock);

  std::vector<Value*> args2;
  args2.push_back(zero);
  args2.push_back(VMObject::classOffset());
  Value* classPtr = GetElementPtrInst::Create(argObj, args2.begin(),
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
  
  BasicBlock* ifFalse = createBasicBlock("cache not ok");
  BranchInst::Create(callBlock, ifFalse, cmp, currentBlock);
  node->addIncoming(cache, currentBlock);
  
  currentBlock = ifFalse;
  Value* newCache = invoke(virtualLookupLLVM, cache, argObj, "", ifFalse, false);
  node->addIncoming(newCache, currentBlock);
  BranchInst::Create(callBlock, currentBlock);

  currentBlock = callBlock;
  Value* methPtr = GetElementPtrInst::Create(node, args1.begin(), args1.end(),
                                         "", currentBlock);

  Value* _meth = new LoadInst(methPtr, "", currentBlock);
  Value* meth = new BitCastInst(_meth, PointerType::getUnqual(funcType), "", currentBlock);
  

  
  std::vector<Value*> args4;
  args4.push_back(zero);
  args4.push_back(five);
  Value* boxedptr = GetElementPtrInst::Create(node, args4.begin(), args4.end(), "", currentBlock);
  Value* boxed = new LoadInst(boxedptr, "", currentBlock);
  /* I put VMArray::llvmType here, but in should be something else... */
  Value* unboxed = new BitCastInst(args[0], VMArray::llvmType, "", currentBlock);
  Value* unboxedptr = GetElementPtrInst::Create(unboxed, args1.begin(), args1.end(), "", currentBlock);
  Value* fakeunboxedptr = new BitCastInst(unboxedptr, args[0]->getType(), "", currentBlock);
  args[0] = SelectInst::Create(boxed, fakeunboxedptr, args[0], "", currentBlock);
  

  Value* ret = invoke(meth, args, "", currentBlock, origMeth->structReturn);


  if (ret->getType() != Type::VoidTy) {
    push(ret);
  }
}

extern "C" CacheNode* virtualLookup(CacheNode* cache, VMObject *obj) {
  Enveloppe* enveloppe = cache->enveloppe;
  VMCommonClass* ocl = obj->classOf;
  VMMethod* orig = enveloppe->originalMethod;
  
  CacheNode* rcache = 0;
  CacheNode* tmp = enveloppe->firstCache;
  CacheNode* last = tmp;
  enveloppe->cacheLock->lock();

  while (tmp) {
    if (ocl == tmp->lastCible) {
      rcache = tmp;
      break;
    } else {
      last = tmp;
      tmp = tmp->next;
    }
  }

  if (!rcache) {
    VMMethod* dmeth = ocl->lookupMethodDontThrow(orig->name,
                                                 orig->parameters,
                                                 false, true);
    if (dmeth == 0) {
      char* methAsciiz = orig->name->UTF8ToAsciiz();
      char* nameAsciiz = orig->classDef->name->UTF8ToAsciiz();
      char* nameSpaceAsciiz = orig->classDef->nameSpace->UTF8ToAsciiz();

      char *buf = (char*)alloca(3 + strlen(methAsciiz) + 
                                    strlen(nameAsciiz) + 
                                    strlen(nameSpaceAsciiz));
      sprintf(buf, "%s.%s.%s", nameSpaceAsciiz, nameAsciiz, methAsciiz);
      const UTF8* newName = VMThread::get()->vm->asciizConstructUTF8(buf);
      dmeth = ocl->lookupMethod(newName, orig->parameters, false, true);
    }
    
    if (cache->methPtr) {
      rcache = CacheNode::allocate();
      rcache->enveloppe = enveloppe;
    } else {
      rcache = cache;
    }
    
    Function* func = dmeth->compiledPtr();
    rcache->methPtr = mvm::jit::executionEngine->getPointerToGlobal(func);
    rcache->lastCible = (VMClass*)ocl;
    rcache->box = (dmeth->classDef->super == N3::pValue);
  }

  if (enveloppe->firstCache != rcache) {
    CacheNode *f = enveloppe->firstCache;
    enveloppe->firstCache = rcache;
    last->next = rcache->next;
    rcache->next = f;

  }
  
  enveloppe->cacheLock->unlock();
  
  return rcache;
}
