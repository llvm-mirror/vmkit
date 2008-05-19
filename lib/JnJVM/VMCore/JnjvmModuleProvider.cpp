//===--- JnjvmModuleProvider.cpp - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"

using namespace llvm;
using namespace jnjvm;

JavaMethod* JnjvmModuleProvider::staticLookup(Class* caller, uint32 index) { 
  JavaCtpInfo* ctpInfo = caller->ctpInfo;
  

  bool isStatic = ctpInfo->isAStaticCall(index);

  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;

  ctpInfo->resolveInterfaceOrMethod(index, cl, utf8, sign);
  
  JavaMethod* meth = cl->lookupMethod(utf8, sign->keyName, isStatic, true);
  
  meth->compiledPtr();
  
  LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
  ctpInfo->ctpRes[index] = LMI->getMethod();

  return meth;
}

bool JnjvmModuleProvider::lookupCallback(Function* F, std::pair<Class*, uint32>& res) {
  callback_iterator CI = callbacks.find(F);
  if (CI != callbacks.end()) {
    res.first = CI->second.first;
    res.second = CI->second.second;
    return true;
  } else {
    return false;
  }
}

bool JnjvmModuleProvider::lookupFunction(Function* F, JavaMethod*& meth) {
  function_iterator CI = functions.find(F);
  if (CI != functions.end()) {
    meth = CI->second;
    return true;
  } else {
    return false;
  }
}

bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (mvm::jit::executionEngine->getPointerToGlobalIfAvailable(F))
    return false;

  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
  
  JavaMethod* meth = 0;
  lookupFunction(F, meth);
  
  if (!meth) {
    // It's a callback
    std::pair<Class*, uint32> p;
    lookupCallback(F, p);
    meth = staticLookup(p.first, p.second); 
  }
  
  void* val = meth->compiledPtr();
  if (F->isDeclaration())
    mvm::jit::executionEngine->updateGlobalMapping(F, val);
  
  if (isVirtual(meth->access)) {
    LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
    uint64_t offset = LMI->getOffset()->getZExtValue();
    ((void**)meth->classDef->virtualVT)[offset] = val;
  }

  return false;
}

void* JnjvmModuleProvider::materializeFunction(JavaMethod* meth) {
  LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
  Function* func = LMI->getMethod();
  if (func->hasNotBeenReadFromBitcode()) {
    // Don't take the JIT lock yet, as Java exceptions in the bytecode must be 
    // loaded first.
    JavaJIT jit;
    jit.compilingClass = meth->classDef;
    jit.compilingMethod = meth;
    jit.module = (JnjvmModule*)TheModule;
    jit.llvmFunction = func;
    if (isNative(meth->access)) {
      jit.nativeCompile();
    } else {
      jit.javaCompile();
    }
  }
  
  return mvm::jit::executionEngine->getPointerToGlobal(func);
}

llvm::Function* JnjvmModuleProvider::addCallback(Class* cl, uint32 index,
                                                 Signdef* sign, bool stat) {
  const llvm::FunctionType* type = 0;
  JnjvmModule* M = cl->isolate->module;
  LLVMSignatureInfo* LSI = M->getSignatureInfo(sign);
  
  if (stat) {
    type = LSI->getStaticType();
  } else {
    type = LSI->getVirtualType();
  }
  Function* func = llvm::Function::Create(type, 
                                          llvm::GlobalValue::GhostLinkage,
                                          "callback",
                                          cl->isolate->module);

  callbacks.insert(std::make_pair(func, std::make_pair(cl, index)));
  return func;
}

void JnjvmModuleProvider::addFunction(Function* F, JavaMethod* meth) {
  functions.insert(std::make_pair(F, meth));
}
